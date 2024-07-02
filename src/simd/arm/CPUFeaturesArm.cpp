#include <asp/simd/base.hpp>

#ifdef ASP_IS_ARM

#include <asp/simd/CPUFeatures.hpp>

namespace asp::simd {

const CPUFeatures& getFeatures() {
    static CPUFeatures features; // there are none!
    return features;
}

}

#endif