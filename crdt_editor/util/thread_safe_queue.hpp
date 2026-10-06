#pragma once

#include <mutex>
#include <queue>
#include <vector>
#include <utility>

template <typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::move(value));
    }

    std::vector<T> take_all() {
        std::lock_guard<std::mutex> lock(mutex);

        std::vector<T> items;
        items.reserve(queue.size());

        while (!queue.empty()) {
            items.push_back(std::move(queue.front()));
            queue.pop();
        }
        return items;
    }

private:
    std::mutex mutex;
    std::queue<T> queue;
};