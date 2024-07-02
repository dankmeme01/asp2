#include <asp/sync/Atomic.hpp>
#include <asp/data/util.hpp>


namespace asp {
    float _u32tof32(uint32_t num) {
        return asp::data::bit_cast<float>(num);
    }

    uint32_t _f32tou32(float num) {
        return asp::data::bit_cast<uint32_t>(num);
    }
}