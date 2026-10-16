#pragma once
#include <asp/detail/Result.hpp>
#include <asp/fs/Metadata.hpp>
#include <sys/stat.h>
#include <fmt/format.h>

namespace asp::fs {

template <typename T>
Result<> wrapCall(auto what, T result) {
    if constexpr (std::is_same_v<T, int> || std::is_same_v<T, ssize_t>) {
        if (result < 0) {
            return Err(fmt::format("{} failed: {}", what, strerror(errno)));
        }
    } else {
        static_assert(std::is_void_v<T>, "wrapCall unsupported type");
    }

    return Ok();
}

static asp::SystemTime toSystemTime(struct timespec ts) {
    asp::Duration dur(ts.tv_sec, ts.tv_nsec);
    return asp::SystemTime::UNIX_EPOCH + dur;
}

static Metadata metadataFromStat(const struct stat& st) {
    Metadata meta;
    if (S_ISREG(st.st_mode)) {
        meta.type = FileType::RegularFile;
    } else if (S_ISDIR(st.st_mode)) {
        meta.type = FileType::Directory;
    } else if (S_ISLNK(st.st_mode)) {
        meta.type = FileType::Symlink;
    } else if (S_ISCHR(st.st_mode)) {
        meta.type = FileType::CharDevice;
    } else if (S_ISBLK(st.st_mode)) {
        meta.type = FileType::BlockDevice;
    } else if (S_ISFIFO(st.st_mode)) {
        meta.type = FileType::Fifo;
    } else if (S_ISSOCK(st.st_mode)) {
        meta.type = FileType::Socket;
    } else {
        meta.type = FileType::Other;
    }

    meta.permissions = fromBits(st.st_mode);

    meta.size = st.st_size;

#if defined(__APPLE__)
    meta.modifiedAt = toSystemTime(st.st_mtimespec);
    meta.accessedAt = toSystemTime(st.st_atimespec);
    meta.createdAt  = toSystemTime(st.st_birthtimespec);
#else
    meta.modifiedAt = toSystemTime(st.st_mtim);
    meta.accessedAt = toSystemTime(st.st_atim);
    // TODO: no creation time, use statx?
#endif

    return meta;
}

}
