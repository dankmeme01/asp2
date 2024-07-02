#include <asp/Log.hpp>

#include <iostream>

namespace asp {

void setLogFunction(std::function<void(LogLevel, const std::string_view)>&& f) {
    getLogFunction() = std::move(f);
}

std::function<void(LogLevel, const std::string_view)>& getLogFunction() {
    static std::function<void(LogLevel, const std::string_view)> func = [](LogLevel level, auto message) {
        std::cerr << "[asp] " << message << std::endl;
    };

    return func;
}

void doLog(LogLevel level, const std::string_view message) {
    getLogFunction()(level, message);
}

}