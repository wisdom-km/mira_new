// CurlHttpClient: Implementation for the DirectorDesk curl module.

#include "CurlHttpClient.h"

#include "DirectorDesk/Core/Error.h"

#include <curl/curl.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace DirectorDesk::Backends {
namespace {

struct CurlWriteState {
    std::vector<std::uint8_t>* memory = nullptr;
    std::ofstream* file = nullptr;
    const std::atomic<bool>* cancel = nullptr;
    std::uint64_t written = 0;
};

std::size_t WriteCallback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    auto* state = static_cast<CurlWriteState*>(userdata);
    if (state->cancel != nullptr && state->cancel->load()) {
        return 0;
    }
    const std::size_t bytes = size * nmemb;
    if (state->file != nullptr) {
        state->file->write(ptr, static_cast<std::streamsize>(bytes));
        if (!state->file->good()) {
            return 0;
        }
    } else if (state->memory != nullptr) {
        state->memory->insert(state->memory->end(), ptr, ptr + bytes);
    }
    state->written += bytes;
    return bytes;
}

int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) {
    auto* request = static_cast<const Platform::HttpGetRequest*>(clientp);
    if (request->cancel != nullptr && request->cancel->load()) {
        return 1;
    }
    if (request->progress) {
        request->progress(static_cast<std::uint64_t>(dlnow < 0 ? 0 : dlnow),
                          static_cast<std::uint64_t>(dltotal < 0 ? 0 : dltotal));
    }
    return 0;
}

Core::Error CurlError(CURLcode code, const std::string& extra) {
    if (code == CURLE_OPERATION_TIMEDOUT) {
        return Core::Error::Make(Core::ErrorCode::IoFailure, extra, "下载超时");
    }
    if (code == CURLE_COULDNT_RESOLVE_HOST || code == CURLE_COULDNT_CONNECT) {
        return Core::Error::Make(Core::ErrorCode::IoFailure, extra, "网络不可达");
    }
    if (code == CURLE_ABORTED_BY_CALLBACK || code == CURLE_WRITE_ERROR) {
        return Core::Error::Make(Core::ErrorCode::IoFailure, extra, "已取消");
    }
    return Core::Error::Make(Core::ErrorCode::IoFailure, extra, "HTTP 错误");
}

bool IsHttps(const std::string& url) {
    return url.rfind("https://", 0) == 0;
}

curl_slist* AppendHeaders(curl_slist* list, const std::vector<Platform::HttpHeader>& headers,
                          const std::string& contentType) {
    std::vector<std::string> lines;
    if (!contentType.empty()) {
        lines.push_back("Content-Type: " + contentType);
    }
    for (const Platform::HttpHeader& header : headers) {
        if (header.name.empty()) {
            continue;
        }
        lines.push_back(header.name + ": " + header.value);
    }
    for (const std::string& line : lines) {
        list = curl_slist_append(list, line.c_str());
    }
    return list;
}

class CurlHttpClient final : public Platform::IHttpClient {
public:
    CurlHttpClient() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~CurlHttpClient() override {
        curl_global_cleanup();
    }

    Core::Result<Platform::HttpGetResponse> Get(const Platform::HttpGetRequest& request) override {
        if (!IsHttps(request.url)) {
            return Core::Result<Platform::HttpGetResponse>::Fail(Core::Error::Make(
                Core::ErrorCode::InvalidArgument, "HTTP client only allows https",
                "只允许 HTTPS 下载"));
        }

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(),
                                                                 curl_easy_cleanup);
        if (!curl) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                Core::Error::Make(Core::ErrorCode::Internal, "curl_easy_init failed", "无法初始化网络"));
        }

        Platform::HttpGetResponse response;
        std::ofstream file;
        CurlWriteState state;
        state.cancel = request.cancel;
        if (!request.outputPath.empty()) {
            file.open(std::filesystem::u8path(request.outputPath),
                      std::ios::binary | std::ios::trunc);
            if (!file) {
                return Core::Result<Platform::HttpGetResponse>::Fail(Core::Error::Make(
                    Core::ErrorCode::IoFailure, "open output failed", "写入失败"));
            }
            state.file = &file;
        } else {
            state.memory = &response.body;
        }

        curl_slist* headerList = AppendHeaders(nullptr, request.headers, {});
        curl_easy_setopt(curl.get(), CURLOPT_URL, request.url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &state);
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &request);
        curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(request.timeoutMs));
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "DirectorDesk/0.1");
        if (headerList != nullptr) {
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headerList);
        }

        const CURLcode code = curl_easy_perform(curl.get());
        if (headerList != nullptr) {
            curl_slist_free_all(headerList);
        }
        if (state.file != nullptr) {
            file.close();
        }
        if (code != CURLE_OK) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                CurlError(code, curl_easy_strerror(code)));
        }
        long status = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
        response.status = static_cast<int>(status);
        response.bytesWritten = state.written;
        return Core::Result<Platform::HttpGetResponse>::Ok(std::move(response));
    }

    Core::Result<Platform::HttpGetResponse> Post(const Platform::HttpPostRequest& request) override {
        if (!IsHttps(request.url)) {
            return Core::Result<Platform::HttpGetResponse>::Fail(Core::Error::Make(
                Core::ErrorCode::InvalidArgument, "HTTP client only allows https",
                "只允许 HTTPS 下载"));
        }
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(),
                                                                 curl_easy_cleanup);
        if (!curl) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                Core::Error::Make(Core::ErrorCode::Internal, "curl_easy_init failed", "无法初始化网络"));
        }

        Platform::HttpGetResponse response;
        CurlWriteState state;
        state.memory = &response.body;
        state.cancel = request.cancel;

        curl_slist* headerList = AppendHeaders(nullptr, request.headers, request.contentType);
        curl_easy_setopt(curl.get(), CURLOPT_URL, request.url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, request.body.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(request.body.size()));
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &state);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(request.timeoutMs));
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "DirectorDesk/0.1");
        if (headerList != nullptr) {
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headerList);
        }

        const CURLcode code = curl_easy_perform(curl.get());
        if (headerList != nullptr) {
            curl_slist_free_all(headerList);
        }
        if (code != CURLE_OK) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                CurlError(code, curl_easy_strerror(code)));
        }
        long status = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
        response.status = static_cast<int>(status);
        response.bytesWritten = state.written;
        return Core::Result<Platform::HttpGetResponse>::Ok(std::move(response));
    }
};

} // namespace

std::unique_ptr<DirectorDesk::Platform::IHttpClient> CreateCurlHttpClient() {
    return std::make_unique<CurlHttpClient>();
}

} // namespace DirectorDesk::Backends
