#include <asp/fs.hpp>
#include <unistd.h>

namespace asp::fs {

Result<Path> Path::tmp() {
    auto tmpdir = getenv("TMPDIR");
    if (tmpdir && *tmpdir) {
        return Ok(Path(tmpdir));
    }

    return Ok(Path{"/data/local/tmp"});
}

}
