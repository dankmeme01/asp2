#include <asp/simd/sse.hpp>

namespace asp::simd {
    float vec128sum(__m128 vec) {
#ifdef __clang__
        return vec[0] + vec[1] + vec[2] + vec[3];
#else
        __m128 sum = _mm_hadd_ps(vec, vec);
        sum = _mm_hadd_ps(sum, sum);

        float result;
        _mm_store_ss(&result, sum);
        return result;
#endif
    }
}
