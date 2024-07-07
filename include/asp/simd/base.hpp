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
