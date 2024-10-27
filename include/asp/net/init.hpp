#pragma once

namespace asp::net {
    // Initialize networking libraries.
    // Note: this is not recommended to be called manually, as it is called automatically by the library.
    void init();
    void cleanup();
}
