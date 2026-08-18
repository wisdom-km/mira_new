#pragma once

#include <functional>
#include <memory>

namespace DirectorDesk::Platform {

class Worker {
public:
    Worker();
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    ~Worker();

    void Start();
    void Submit(std::function<void()> job);
    void Shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace DirectorDesk::Platform
