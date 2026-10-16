#pragma once
#include <cstdint>

namespace asp::fs {

enum class Permissions : uint32_t {
    None = 0,

    OtherExec = 0001,
    OtherWrite = 0002,
    OtherRead = 0004,
    GroupExec = 0010,
    GroupWrite = 0020,
    GroupRead = 0040,
    OwnerExec = 0100,
    OwnerWrite = 0200,
    OwnerRead = 0400,

    // POSIX only
    SetUid = 04000,
    SetGid = 02000,
    Sticky = 01000,

    OwnerAll = OwnerRead | OwnerWrite | OwnerExec,
    GroupAll = GroupRead | GroupWrite | GroupExec,
    OtherAll = OtherRead | OtherWrite | OtherExec,
    All = OwnerAll | GroupAll | OtherAll,
};

constexpr Permissions operator|(Permissions a, Permissions b) {
    return Permissions(uint32_t(a) | uint32_t(b));
}

constexpr Permissions operator&(Permissions a, Permissions b) {
    return Permissions(uint32_t(a) & uint32_t(b));
}

constexpr Permissions& operator|=(Permissions& a, Permissions b) {
    return a = a | b;
}

constexpr Permissions& operator&=(Permissions& a, Permissions b) {
    return a = a & b;
}

constexpr Permissions operator~(Permissions a) {
    return Permissions(~uint32_t(a));
}

constexpr bool hasPermission(Permissions perms, Permissions check) {
    return (perms & check) == check;
}

constexpr uint32_t toBits(Permissions perms) {
    return uint32_t(perms) & 07777;
}

constexpr Permissions fromBits(uint32_t bits) {
    return Permissions(bits & 07777);
}

}
