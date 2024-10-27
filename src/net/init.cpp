#include <asp/net/init.hpp>
#include "utils.hpp"

#include <asp/Log.hpp>

namespace asp::net {
#ifdef ASP_IS_WIN
    void init() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            asp::log(LogLevel::Error, Error::lastOsError().message());
            asp::detail::assertionFail("failed to initialize winsock");
        }
    }

    void cleanup() {
        WSACleanup();
    }
#else
    void init() {}
    void cleanup() {}
#endif
}

namespace {
    // This ensures that winsock is initialized when the library is loaded, and cleaned up when it is unloaded.
    struct NetInit {
        NetInit() {
            asp::net::init();
        }

        ~NetInit() {
            asp::net::cleanup();
        }
    } g_aspNetInit;
}
