#include <asp/thread/Thread.hpp>
#include <asp/detail/config.hpp>

#ifdef ASP_IS_WIN

#include <Windows.h>

// Taken from geode
// https://learn.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2022
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void obliterate(const std::string& name) {
    // exception
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name.c_str();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(0x406d1388, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { }
#pragma warning(pop)
}

void asp::_setThreadName(const std::string& name) {
    obliterate(name);
}

#elif defined(__APPLE__)

void asp::_setThreadName(const std::string& name) {
    pthread_setname_np(name.c_str());
}

#else

void asp::_setThreadName(const std::string& name) {
    pthread_setname_np(pthread_self(), name.c_str());
}

#endif
