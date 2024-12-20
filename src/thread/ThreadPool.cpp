#include <asp/thread/ThreadPool.hpp>
#include <asp/Log.hpp>

namespace asp {

ThreadPool::ThreadPool(size_t tc) : _storage(std::make_shared<Storage>()) {
    for (size_t i = 0; i < tc; i++) {
        Thread<> thread;
        thread.setLoopFunction([storage = _storage, i = i](auto&) {
            auto& worker = storage->workers.at(i);

            auto task = storage->taskQueue.popTimeout(time::Duration::fromMillis(10));

            if (!task) return;

            worker.doingWork = true;

            task.value()();

            worker.doingWork = false;
        });

        Worker worker = {
            .thread = std::move(thread),
            .doingWork = false
        };

        _storage->workers.emplace_back(std::move(worker));
    }

    for (auto& worker : _storage->workers) {
        worker.thread.start();
    }
}

ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {}

ThreadPool::~ThreadPool() {
    // if taskQueue is null, this instance of ThreadPool was moved from.
    if (!_storage) return;

    try {
        this->join();

        // stop all threads and wait for them to terminate
        for (auto& worker : _storage->workers) {
            worker.thread.stop();
        }

        for (auto& worker : _storage->workers) {
            worker.thread.join();
        }

        _storage->workers.clear();
    } catch (const std::exception& e) {
        asp::log(LogLevel::Error, std::string("failed to cleanup thread pool: ") + e.what());
    }
}

void ThreadPool::pushTask(const Task& task) {
    this->_checkValid();

    _storage->taskQueue.push(task);
}

void ThreadPool::pushTask(Task&& task) {
    this->_checkValid();

    _storage->taskQueue.push(std::move(task));
}

void ThreadPool::join() {
    this->_checkValid();

    while (!_storage->taskQueue.empty()) {
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    // wait for working threads to finish
    bool stillWorking;

    do {
        std::this_thread::sleep_for(std::chrono::microseconds(200));
        stillWorking = false;
        for (const auto& worker : _storage->workers) {
            if (worker.doingWork) {
                stillWorking = true;
                break;
            }
        }
    } while (stillWorking);
}

bool ThreadPool::isDoingWork() {
    this->_checkValid();

    if (!_storage->taskQueue.empty()) return true;

    for (const auto& worker : _storage->workers) {
        if (worker.doingWork) {
            return true;
        }
    }

    return false;
}

void ThreadPool::setExceptionFunction(const std::function<void(const std::exception&)>& f) {
    this->_checkValid();

    _storage->onException = f;

    for (auto& worker : _storage->workers) {
        worker.thread.setExceptionFunction(f);
    }
}

void ThreadPool::_checkValid() {
    if (!_storage) throw std::runtime_error("Attempting to use an invalid ThreadPool that was moved from");
}

}