#include <asp/simd/sse.hpp>

#ifdef ASP_IS_X86

namespace asp::simd {
    float vec128sum(__m128 vec) {
        __m128 sum = _mm_hadd_ps(vec, vec);
        sum = _mm_hadd_ps(sum, sum);

        float result;
        _mm_store_ss(&result, sum);
        return result;
    }
}

#endif
