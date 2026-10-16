#include <asp/fs.hpp>
#include <unistd.h>
#include <copyfile.h>

namespace asp::fs {

Result<Path> Path::tmp() {
    auto tmpdir = getenv("TMPDIR");
    if (tmpdir && *tmpdir) {
        return Ok(Path(tmpdir));
    }

    char buf[1024];
    if (!confstr(_CS_DARWIN_USER_TEMP_DIR, buf, sizeof(buf)) || buf[0] == '\0') {
        return Err("Unable to determine temporary directory");
    }

    return Ok(Path{buf});
}

Result<void> copy(PathView origin, PathView dest) {
    auto result = copyfile(origin.string().c_str(), dest.string().c_str(), nullptr, COPYFILE_ALL);
    if (result < 0) {
        return Err(fmt::format("Unable to copy file: {}", strerror(errno)));
    }
    return Ok();
}

}
