#include <asp/detail/config.hpp>
#include <asp/Log.hpp>
#include <stdexcept>
#include <string>

void asp::detail::assertionFail(const char* message) {
    auto msg = std::string("asp assertion failed: ") + message;

    asp::log(LogLevel::Error, msg);
    throw std::runtime_error(msg);
}
