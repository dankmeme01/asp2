#pragma once

#include "../detail/config.hpp"
#include <thread>
#include <atomic>

namespace asp {

class DeadlockGuard {
public:
    void lockAttempt();
    void lockSuccess();
    void unlock();

private:
    std::atomic<std::thread::id> current;
};

}