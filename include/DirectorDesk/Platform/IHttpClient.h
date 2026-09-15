// IHttpClient: Public or internal interface for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include "DirectorDesk/Core/Result.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace DirectorDesk::Platform {

struct HttpHeader {
    std::string name;
    std::string value;
};

struct HttpGetRequest {
    std::string url;
    std::string outputPath;
    std::uint32_t timeoutMs = 30000;
    const std::atomic<bool>* cancel = nullptr;
    std::function<void(std::uint64_t downloaded, std::uint64_t total)> progress;
    std::vector<HttpHeader> headers;
};

struct HttpPostRequest {
    std::string url;
    std::string body;
    std::string contentType = "application/json";
    std::vector<HttpHeader> headers;
    std::uint32_t timeoutMs = 120000;
    const std::atomic<bool>* cancel = nullptr;
};

struct HttpGetResponse {
    int status = 0;
    std::vector<std::uint8_t> body;
    std::uint64_t bytesWritten = 0;
};

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual Core::Result<HttpGetResponse> Get(const HttpGetRequest& request) = 0;
    virtual Core::Result<HttpGetResponse> Post(const HttpPostRequest& request) = 0;
};

} // namespace DirectorDesk::Platform
