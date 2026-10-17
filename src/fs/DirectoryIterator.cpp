#include <asp/fs/DirectoryIterator.hpp>

namespace asp::fs {

// Recursive directory iterator, implemented in terms of a stack of regular directory iterators

std::optional<Result<DirectoryEntry>> RecursiveDirectoryIterator::next() {
    while (!m_stack.empty()) {
        auto opt = m_stack.back().next();
        if (!opt) {
            // this directory is exhausted, pop it off the stack and continue
            m_stack.pop_back();
            continue;
        }

        if (opt->isErr()) {
            return opt;
        }

        auto entry = std::move(*opt).unwrap();
        if (entry.isDir()) {
            // push the new directory onto the stack
            GEODE_UNWRAP_INTO(auto newIter, DirectoryIterator::create(entry.path));
            m_stack.emplace_back(std::move(newIter));

            // and yield it
            return Ok(std::move(entry));
        }

        // else, it's not a directory so we can just yield it
        return Ok(std::move(entry));
    }

    return std::nullopt;
}

Result<RecursiveDirectoryIterator> RecursiveDirectoryIterator::create(PathView path) {
    GEODE_UNWRAP_INTO(auto it, DirectoryIterator::create(path));

    return Ok(RecursiveDirectoryIterator {
        path.toPath(),
        std::move(it)
    });
}

}
