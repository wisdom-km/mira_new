#include "DirectorDesk/Platform/Worker.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

namespace DirectorDesk::Platform {

struct Worker::Impl {
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<std::function<void()>> jobs;
    std::thread thread;
    bool stop = false;
    bool started = false;
};

Worker::Worker() : m_impl(std::make_unique<Impl>()) {}

Worker::~Worker() {
    Shutdown();
}

void Worker::Start() {
    if (m_impl->started) {
        return;
    }
    m_impl->started = true;
    m_impl->thread = std::thread([this]() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(m_impl->mutex);
                m_impl->cv.wait(lock, [this]() { return m_impl->stop || !m_impl->jobs.empty(); });
                if (m_impl->stop && m_impl->jobs.empty()) {
                    return;
                }
                job = std::move(m_impl->jobs.front());
                m_impl->jobs.pop_front();
            }
            job();
        }
    });
}

void Worker::Submit(std::function<void()> job) {
    if (!job || !m_impl->started) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        if (m_impl->stop) {
            return;
        }
        m_impl->jobs.push_back(std::move(job));
    }
    m_impl->cv.notify_one();
}

void Worker::Shutdown() {
    if (m_impl == nullptr || !m_impl->started) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->stop = true;
    }
    m_impl->cv.notify_one();
    if (m_impl->thread.joinable()) {
        m_impl->thread.join();
    }
    m_impl->started = false;
}

} // namespace DirectorDesk::Platform
