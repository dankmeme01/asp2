#pragma once
#include "Mutex.hpp"

#include <condition_variable>
#include <queue>
#include <optional>

#include <asp/time/Duration.hpp>
#include <asp/time/chrono.hpp>

namespace asp {

/// Thread-safe message queue for exchanging data between multiple threads.
/// Can have multiple senders and receivers.
template <typename T>
class Channel {
public:
    Channel() {}

    bool empty() const {
        std::unique_lock lock(mtx);
        return queue.empty();
    }

    size_t size() const {
        std::unique_lock lock(mtx);
        return queue.size();
    }

    // Obtains the element at the front of the queue, if the channel is empty, blocks until there's data.
    T pop() {
        std::unique_lock lock(mtx);
        if (!queue.empty()) {
            return doPop(queue);
        }

        cvar.wait(lock, [this] { return !queue.empty(); });

        return doPop(queue.data);
    }

    // Like `pop`, but will return `std::nullopt` if the given timeout expires before there's available data.
    std::optional<T> popTimeout(const time::Duration& timeout) {
        return popTimeout(time::toChrono<std::chrono::microseconds>(timeout));
    }

    // Like `pop`, but will return `std::nullopt` if the given timeout expires before there's available data.
    template <typename Rep, typename Period>
    std::optional<T> popTimeout(std::chrono::duration<Rep, Period> timeout) {
        std::unique_lock lock(mtx);
        if (!queue.empty()) {
            return doPop(queue);
        }

        bool available = cvar.wait_for(lock, timeout, [this] { return !queue.empty(); });

        if (!available) {
            return std::nullopt;
        }

        return std::optional<T>(std::move(doPop(queue)));
    }

    // Blocks until messages are available, does not actually pop any messages from the channel.
    void waitForMessages(const time::Duration& timeout) {
        waitForMessages(time::toChrono<std::chrono::microseconds>(timeout));
    }

    // Blocks until messages are available, does not actually pop any messages from the channel.
    template <typename Rep, typename Period>
    void waitForMessages(std::chrono::duration<Rep, Period> timeout) {
        std::unique_lock lock(mtx);

        if (!queue.empty()) {
            return;
        }

        cvar.wait_for(lock, timeout, [this] { return !queue.empty(); });
    }

    // Obtains the element at the front of the queue, throws if the channel is empty.
    T popNow() {
        std::unique_lock lock(mtx);
        if (queue.empty()) {
            throw std::runtime_error("attempting to pop a message from an empty channel");
        }

        return doPop(queue);
    }

    // Returns the element at the front of the queue if present, otherwise returns `std::nullopt`.
    std::optional<T> tryPop() {
        std::unique_lock lock(mtx);
        if (queue.empty()) return std::nullopt;

        return doPop(queue);
    }

    // Pushes a new message to the queue.
    void push(const T& msg) {
        std::unique_lock lock(mtx);
        queue.push(msg);
        cvar.notify_one();
    }

    // Pushes a new message to the queue.
    void push(T&& msg) {
        std::unique_lock lock(mtx);
        queue.push(std::move(msg));
        cvar.notify_one();
    }

private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable cvar;

    T doPop(std::queue<T>& q) {
        T val = std::move(q.front());
        q.pop();
        return val;
    }
};

}