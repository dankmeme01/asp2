#pragma once
#include "base.hpp"

namespace asp::simd {
    struct CPUFeaturesX86 {
        bool sse3, pclmulqdq, ssse3, sse4_1, sse4_2, aes, avx, sse, sse2, avx2, avx512, avx512dq;
    };

    struct CPUFeaturesArm {};

#ifdef ASP_IS_X86
    using CPUFeatures = CPUFeaturesX86;
#else
    using CPUFeatures = CPUFeaturesArm;
#endif

    const CPUFeatures& getFeatures();
}
