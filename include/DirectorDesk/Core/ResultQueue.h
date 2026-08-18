#pragma once

#include <deque>
#include <mutex>
#include <utility>

namespace DirectorDesk::Core {

template <typename T>
class ResultQueue {
public:
    void Push(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_items.push_back(std::move(value));
    }

    bool TryPop(T& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_items.empty()) {
            return false;
        }
        out = std::move(m_items.front());
        m_items.pop_front();
        return true;
    }

private:
    std::mutex m_mutex;
    std::deque<T> m_items;
};

} // namespace DirectorDesk::Core
