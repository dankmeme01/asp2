#include <asp/fs.hpp>
#include <unistd.h>

namespace asp::fs {

Result<Path> Path::tmp() {
    auto tmpdir = getenv("TMPDIR");
    if (tmpdir && *tmpdir) {
        return Ok(Path(tmpdir));
    }

    char buf[1024];
    confstr(_CS_DARWIN_USER_TEMP_DIR, buf, sizeof(buf));
    if (buf[0] == '\0') {
        return Err("Unable to determine temporary directory");
    }
    return Ok(Path{buf});
}

}
