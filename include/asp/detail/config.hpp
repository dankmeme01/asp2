#pragma once

#define ASP_ALWAYS_ASSERT(cond, message) if(!(cond)) [[unlikely]] ::asp::detail::assertionFail(message)

#if (!defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG) || defined(ASP_ENABLE_DEBUG)) && !defined(ASP_NO_DEBUG)
# define ASP_DEBUG
# define ASP_ASSERT(cond, message) ASP_ALWAYS_ASSERT(cond, message)
#else
# define ASP_ASSERT(cond, message) ((void)(cond))
#endif

namespace asp::detail {
    [[noreturn]] void assertionFail(const char* message);
}
