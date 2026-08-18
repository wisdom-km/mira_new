// Worker: Public or internal interface for the DirectorDesk Platform module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.

#pragma once

#include <functional>
#include <memory>

namespace DirectorDesk::Platform {

class Worker {
public:
    // Owns one background thread used for blocking I/O and asset work.
    Worker();
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    ~Worker();

    // Starts the worker once; subsequent calls are no-ops.
    void Start();
    // Enqueues work without exposing the worker thread to callers.
    void Submit(std::function<void()> job);
    // Stops accepting work, drains the queue, and joins the thread.
    void Shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace DirectorDesk::Platform
