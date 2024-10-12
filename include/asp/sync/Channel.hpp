#pragma once
#include "Mutex.hpp"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/lock_types.hpp>
#include <queue>
#include <optional>

#include <asp/time/Duration.hpp>

namespace asp {

/// Thread-safe message queue for exchanging data between multiple threads.
/// Can have multiple senders and receivers.
template <typename T>
class Channel {
public:
    Channel() {}

    bool empty() const {
        return queue.lock()->empty();
    }

    size_t size() const {
        return queue.lock()->size();
    }

    // Obtains the element at the front of the queue, if the channel is empty, blocks until there's data.
    T pop() {
        boost::unique_lock lock(queue.mtx);
        if (!queue.data.empty()) {
            return doPop(queue.data);
        }

        cvar.wait(lock, [this] { return !queue.data.empty(); });

        return doPop(queue.data);
    }

    // Like `pop`, but will return `std::nullopt` if the given timeout expires before there's available data.
    std::optional<T> popTimeout(const time::Duration& timeout) {
        return popTimeout(boost::chrono::microseconds(timeout.micros()));
    }

    // Like `pop`, but will return `std::nullopt` if the given timeout expires before there's available data.
    template <typename Rep, typename Period>
    std::optional<T> popTimeout(boost::chrono::duration<Rep, Period> timeout) {
        boost::unique_lock lock(queue.mtx);
        if (!queue.data.empty()) {
            return doPop(queue.data);
        }

        bool available = cvar.wait_for(lock, timeout, [this] { return !queue.data.empty(); });

        if (!available) {
            return std::nullopt;
        }

        return std::optional<T>(std::move(doPop(queue.data)));
    }

    // Blocks until messages are available, does not actually pop any messages from the channel.
    void waitForMessages(const time::Duration& timeout) {
        waitForMessages(boost::chrono::microseconds(timeout.micros()));
    }

    // Blocks until messages are available, does not actually pop any messages from the channel.
    template <typename Rep, typename Period>
    void waitForMessages(boost::chrono::duration<Rep, Period> timeout) {
        boost::unique_lock lock(queue.mtx);

        if (!queue.data.empty()) {
            return;
        }

        cvar.wait_for(lock, timeout, [this] { return !queue.data.empty(); });
    }

    // Obtains the element at the front of the queue, throws if the channel is empty.
    T popNow() {
        auto q = queue.lock();
        if (q->empty()) {
            throw std::runtime_error("attempting to pop a message from an empty channel");
        }

        return doPop(*q);
    }

    // Returns the element at the front of the queue if present, otherwise returns `std::nullopt`.
    std::optional<T> tryPop() {
        auto q = queue.lock();
        if (q->empty()) return std::nullopt;

        return doPop(*q);
    }

    // Pushes a new message to the queue.
    void push(const T& msg) {
        queue.lock()->push(msg);
        cvar.notify_one();
    }

    // Pushes a new message to the queue.
    void push(T&& msg) {
        queue.lock()->push(std::move(msg));
        cvar.notify_one();
    }

private:
    Mutex<std::queue<T>> queue;
    boost::condition_variable cvar;

    T doPop(std::queue<T>& q) {
        T val = std::move(q.front());
        q.pop();
        return val;
    }
};

}