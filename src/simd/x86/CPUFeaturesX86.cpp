#include <asp/simd/base.hpp>

#ifdef ASP_IS_X86

#include <asp/simd/CPUFeatures.hpp>
static void cpuid(int info[4], int infoType) {
#ifdef GEODE_IS_WINDOWS
    __cpuid(info, infoType);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3])
        : "a" (infoType)
    );
#endif
}

namespace asp::simd {

const CPUFeatures& getFeatures() {
#define FEATURE(where, name, bit) features.name = (where & (1 << bit)) != 0
    static CPUFeatures features = [] {
        CPUFeatures features;
        int arr[4];
        cpuid(arr, 1);

        int eax = arr[0];
        int ebx = arr[1];
        int ecx = arr[2];
        int edx = arr[3];

        FEATURE(ecx, sse3, 0);
        FEATURE(ecx, pclmulqdq, 1);
        FEATURE(ecx, ssse3, 9);
        FEATURE(ecx, sse4_1, 19);
        FEATURE(ecx, sse4_2, 20);
        FEATURE(ecx, aes, 25);
        FEATURE(ecx, avx, 28);
        FEATURE(edx, sse, 25);
        FEATURE(edx, sse2, 26);

        // call again with eax = 7
        {
            int arr[4];
            arr[2] = 0;
            cpuid(arr, 7);

            int eax = arr[0];
            int ebx = arr[1];
            int ecx = arr[2];
            int edx = arr[3];

            FEATURE(ebx, avx2, 5);
            FEATURE(ebx, avx512, 16);
            FEATURE(ebx, avx512dq, 17);
        }

        return features;
    }();

    return features;
#undef FEATURE
}

}

#endif