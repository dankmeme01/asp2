#pragma once

#include "../detail/config.hpp"
#include "../sync/Atomic.hpp"
#include "../Log.hpp"

#include <functional>
#include <memory>
#include <thread>
#include <stdexcept>

namespace asp {

template <typename... TFuncArgs>
class Thread {
public:
    struct Storage;

    // Stop token
    class StopToken {
    public:
        StopToken(std::shared_ptr<Storage> p) : storage(std::move(p)) {}
        StopToken(StopToken&&) = default;
        StopToken& operator=(StopToken&&) = default;

        // Stops the thread (not immediately, but the loop function will never be called again after it returns)
        void stop() {
            storage->_stopped.set();
        }

    private:
        std::shared_ptr<Storage> storage;
    };

    using TFunc = std::function<void (TFuncArgs..., StopToken&)>;

    Thread() {
        _storage = std::make_shared<Storage>();
    }

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Thread(Thread&& other) noexcept {
        _handle = std::move(other._handle);
        _storage = std::move(other._storage);
        other.movedFrom = true;
    }

    Thread& operator=(Thread&& other) noexcept {
        if (this != &other) {
            _handle = std::move(other._handle);
            _storage = std::move(other._storage);
            other.movedFrom = true;
        }
    }

    Thread(TFunc&& func) {
        _storage = std::make_shared<Storage>();
        this->setLoopFunction(std::move(func));
    }

    void setLoopFunction(TFunc&& func) {
        _storage->loopFunc = std::move(func);
    }

    void start(TFuncArgs&&... args) requires (sizeof...(TFuncArgs) > 0) {
        _storage->_stopped.clear();
        _handle = std::thread([_storage = _storage](TFuncArgs&&... args) {
            if (_storage->onStart) {
                _storage->onStart();
            }

            StopToken stopToken(_storage);

            try {
                while (!_storage->_stopped) {
                    _storage->loopFunc(args..., stopToken);
                }
            } catch (const std::exception& e) {
                if (_storage->onException) {
                    _storage->onException(e);
                } else {
                    asp::log(LogLevel::Error, std::string("unhandled exception from a Thread: ") + e.what());
                    throw;
                }
            }

            if (_storage->onTermination) {
                _storage->onTermination();
            }
        }, std::forward<TFuncArgs>(args)...);
    }

    void start(const TFuncArgs&... args) {
        _storage->_stopped.clear();
        _handle = std::thread([_storage = _storage](TFuncArgs&&... args) {
            if (_storage->onStart) {
                _storage->onStart();
            }

            StopToken stopToken(_storage);

            try {
                while (!_storage->_stopped) {
                    _storage->loopFunc(args..., stopToken);
                }
            } catch (const std::exception& e) {
                if (_storage->onException) {
                    _storage->onException(e);
                } else {
                    asp::log(LogLevel::Error, std::string("unhandled exception from a Thread: ") + e.what());
                    throw;
                }
            }

            if (_storage->onTermination) {
                _storage->onTermination();
            }
        }, args...);
    }

    // Request the thread to be stopped as soon as possible
    void stop() {
        if (_storage) {
            _storage->_stopped.set();
        } else {
            asp::log(LogLevel::Error, "tried to stop a detached Thread");
            throw std::runtime_error("tried to stop a detached Thread");
        }
    }

    // Join the thread if possible, else do nothing
    void join() {
        if (_handle.joinable()) _handle.join();
    }

    // Stop the thread and wait for it to terminate
    void stopAndWait() {
        this->stop();
        this->join();
    }

    // Detach the thread and let it execute even after this `Thread` object is destructed. This `Thread` instance must not be used afterwards.
    // NOTE: this intentionally leaks resources. The thread will be impossible to stop unless an exception occurs.
    void detach() {
        if (_handle.joinable()) _handle.detach();
        _handle = {};
        // release the ownership
        _storage.reset();
    }

    // Set the function that will be called when the thread is started. It will be called from within the created thread.
    void setStartFunction(std::function<void()>&& f) {
        _storage->onStart = std::move(f);
    }

    // Set the function that will be called when the thread is started. It will be called from within the created thread.
    void setStartFunction(const std::function<void()>& f) {
        _storage->onStart = f;
    }

    // Set the function that will be called when an exception is thrown. If not set, the exception will be rethrown and the program will be terminated.
    void setExceptionFunction(std::function<void(const std::exception&)>&& f) {
        _storage->onException = std::move(f);
    }

    // Set the function that will be called when an exception is thrown. If not set, the exception will be rethrown and the program will be terminated.
    void setExceptionFunction(const std::function<void(const std::exception&)>& f) {
        _storage->onException = f;
    }

    // Set the function that will be called when the thread is about to terminate. It will be called from within the created thread.
    void setTerminationFunction(std::function<void()>&& f) {
        _storage->onTermination = std::move(f);
    }

    // Set the function that will be called when the thread is about to terminate. It will be called from within the created thread.
    void setTerminationFunction(const std::function<void()>& f) {
        _storage->onTermination = f;
    }

    bool isStopped() {
        return !_storage || _storage->_stopped;
    }

    ~Thread() {
        if (!movedFrom && _storage != nullptr) {
            this->stopAndWait();
        }
    }

    struct Storage {
        AtomicFlag _stopped;
        TFunc loopFunc;
        std::function<void()> onStart;
        std::function<void()> onTermination;
        std::function<void(const std::exception&)> onException;
    };

private:
    std::thread _handle;
    std::shared_ptr<Storage> _storage = nullptr;
    bool movedFrom = false;
};

template <typename... Args>
using StopToken = Thread<Args...>::StopToken;

}