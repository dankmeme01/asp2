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

#if defined(__x86_64__) || defined(_M_X64)
# define ASP_IS_X86
# define ASP_IS_X64
#elif defined(__x86__) || defined(__i386__) || defined(_M_IX86)
# define ASP_IS_X86
#else
# define ASP_IS_ARM
# if UINTPTR_MAX > 0xffffffff
#  define ASP_IS_ARM64
# endif
#endif


#if UINTPTR_MAX > 0xffffffff
# define ASP_IS_64BIT
#endif

// Platform

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__) || defined(WIN64) || defined(_WIN64) || defined(__WIN64) && !defined(__CYGWIN__)
# define ASP_IS_WIN
# if defined(WIN64) || defined(_WIN64) || defined(__WIN64) && !defined(__CYGWIN__)
#  define ASP_IS_WIN64
# else
#  define ASP_IS_WIN32
# endif
#endif

#if defined(__APPLE__)
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE
#  define ASP_IS_IOS
# else
#  define ASP_IS_MACOS
# endif
#endif

#ifdef __clang__
# define ASP_CLANG_CONSTEXPR constexpr inline
#else
# define ASP_CLANG_CONSTEXPR inline
#endif

#if !defined(__clang__) && !defined(__has_builtin)
# define __has_builtin(x) 0
#endif

#ifdef _WIN32
# define ASP_FORCE_INLINE __forceinline
#else
# define ASP_FORCE_INLINE inline __attribute__((always_inline))
#endif
