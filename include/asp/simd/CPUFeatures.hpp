#pragma once

namespace asp::simd {
    struct CPUFeaturesX86 {
        bool sse3, pclmulqdq, ssse3, sse4_1, sse4_2, aes, avx, sse, sse2, avx2, avx512, avx512dq;
    };

    struct CPUFeaturesArm {};

#if defined(__x86__) || defined(__x86_64__)
    using CPUFeatures = CPUFeaturesX86;
#else
    using CPUFeatures = CPUFeaturesArm;
#endif

    const CPUFeatures& getFeatures();
}
