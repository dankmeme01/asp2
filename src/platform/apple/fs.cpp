#include <asp/fs.hpp>
#include <unistd.h>
#include <copyfile.h>
#include <sys/uio.h>
#include <sys/attr.h>
#include <sys/clonefile.h>
#include <sys/socket.h>

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

Result<void> copy(PathView origin_, PathView dest_) {
    auto origin = origin_.string();
    auto dest = dest_.string();

    if (clonefile(origin.c_str(), dest.c_str(), 0) == 0) {
        return Ok();
    } else {
        int e = errno;
        if (e != EXDEV) {
            return Err(fmt::format("clonefile failed: {}", strerror(e)));
        }
    }

    auto result = copyfile(origin.c_str(), dest.c_str(), nullptr, COPYFILE_ALL);
    if (result < 0) {
        return Err(fmt::format("copyfile failed: {}", strerror(errno)));
    }
    return Ok();
}

}
