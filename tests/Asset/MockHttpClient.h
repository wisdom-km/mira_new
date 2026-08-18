// MockHttpClient: Public or internal interface for the DirectorDesk Asset module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Platform/IHttpClient.h"

#include "DirectorDesk/Core/Error.h"
#include "DirectorDesk/Platform/Paths.h"

#include <unordered_map>

namespace DirectorDesk::Tests {

class MockHttpClient final : public Platform::IHttpClient {
public:
    struct Route {
        int status = 200;
        std::vector<std::uint8_t> body;
        bool timeout = false;
        bool unreachable = false;
        bool writeFail = false;
    };

    std::unordered_map<std::string, Route> routes;

    Core::Result<Platform::HttpGetResponse> Get(const Platform::HttpGetRequest& request) override {
        if (request.cancel != nullptr && request.cancel->load()) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                Core::Error::Make(Core::ErrorCode::IoFailure, "cancelled", "已取消"));
        }
        const auto found = routes.find(request.url);
        if (found == routes.end()) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                Core::Error::Make(Core::ErrorCode::IoFailure, "missing route", "网络不可达"));
        }
        const Route& route = found->second;
        if (route.timeout) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                Core::Error::Make(Core::ErrorCode::IoFailure, "timeout", "下载超时"));
        }
        if (route.unreachable) {
            return Core::Result<Platform::HttpGetResponse>::Fail(
                Core::Error::Make(Core::ErrorCode::IoFailure, "unreachable", "网络不可达"));
        }
        if (request.progress) {
            request.progress(route.body.size(), route.body.size());
        }
        Platform::HttpGetResponse response;
        response.status = route.status;
        response.bytesWritten = route.body.size();
        if (!request.outputPath.empty()) {
            if (route.writeFail) {
                return Core::Result<Platform::HttpGetResponse>::Fail(
                    Core::Error::Make(Core::ErrorCode::IoFailure, "write fail", "写入失败"));
            }
            auto written = Platform::Paths::WriteBinaryFile(request.outputPath, route.body.data(),
                                                            route.body.size());
            if (!written.IsOk()) {
                return Core::Result<Platform::HttpGetResponse>::Fail(written.GetError());
            }
        } else {
            response.body = route.body;
        }
        return Core::Result<Platform::HttpGetResponse>::Ok(std::move(response));
    }
};

} // namespace DirectorDesk::Tests
