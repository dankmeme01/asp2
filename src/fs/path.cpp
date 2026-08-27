#include <asp/fs/path.hpp>

namespace asp::fs {

// Shared impl

size_t _impl::hash(std::string_view path) noexcept {
    return std::hash<std::string_view>()(path);
}

std::optional<PathView> _impl::parent(std::string_view path) noexcept {
    auto slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return std::nullopt;
    }
    return PathView(path.substr(0, slash));
}

std::optional<std::string_view> _impl::filename(std::string_view path) noexcept {
    auto slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return path;
    }
    if (slash + 1 >= path.size()) {
        return std::nullopt;
    }
    return path.substr(slash + 1);
}

std::optional<std::string_view> _impl::extension(std::string_view path) noexcept {
    auto fn = _impl::filename(path);
    if (!fn) return std::nullopt;

    auto dot = fn->find_last_of('.');
    if (dot == std::string::npos) return std::nullopt;

    return fn->substr(dot + 1);
}

bool _impl::equal(std::string_view lhs, std::string_view rhs) noexcept {
    return lhs == rhs;
}

std::strong_ordering _impl::compare(std::string_view lhs, std::string_view rhs) noexcept {
    return lhs <=> rhs;
}

// PathView

PathView::operator std::string_view() const noexcept {
    return m_path;
}

std::string_view PathView::view() const noexcept {
    return m_path;
}

std::string PathView::string() const {
    return std::string(m_path);
}

Path PathView::toPath() const {
    return Path(m_path);
}

// -- Path -- //

Path& Path::assign(PathView path) {
    m_path.assign(path);
    return *this;
}

Path& Path::assign(std::string&& path) noexcept {
    m_path = std::move(path);
    return *this;
}

Path& Path::operator/=(PathView path) {
    return this->append(path);
}

Path& Path::append(PathView path) {
    if (path.empty()) return *this;

    bool startsWithSep = path.view().starts_with("/") || path.view().starts_with("\\");
    if (path.isAbsolute() || startsWithSep) {
        m_path.assign(path);
        return *this;
    }

    if (!m_path.empty() && !m_path.ends_with("/") && !m_path.ends_with("\\")) {
        m_path += "/";
    }
    m_path += path.view();

    return *this;
}

Path& Path::operator+=(PathView path) {
    return this->concat(path);
}

Path& Path::concat(PathView path) {
    m_path += path.view();
    return *this;
}

void Path::clear() noexcept {
    m_path.clear();
}

void Path::reserve(size_t newCapacity) {
    m_path.reserve(newCapacity);
}

void Path::shrinkToFit() {
    m_path.shrink_to_fit();
}

Path& Path::makePreferred() noexcept {
    for (auto& c : m_path) {
        if (c == '/' || c == '\\') {
            c = PreferredSep;
        }
    }
    return *this;
}

Path& Path::removeFilename() {
    if (auto fn = this->filename()) {
        m_path.erase(m_path.size() - fn->size());
    }
    return *this;
}

Path& Path::replaceFilename(std::string_view filename) {
    this->removeFilename();
    m_path += filename;
    return *this;
}

Path& Path::replaceExtension(std::string_view extension) {
    auto dotPos = m_path.find_last_of('.');
    if (dotPos != std::string::npos) {
        m_path.erase(dotPos);
    }

    if (extension.starts_with('.')) {
        extension.remove_prefix(1);
    }

    m_path.push_back('.');
    m_path += extension;

    return *this;
}

std::string Path::string() const& {
    return m_path;
}

std::string Path::string() && {
    return std::move(m_path);
}

PathView Path::view() const noexcept {
    return PathView(m_path);
}

const char* Path::c_str() const noexcept {
    return m_path.c_str();
}

Path::operator PathView() const noexcept {
    return this->view();
}

Path operator/(const PathView& lhs, const PathView& rhs) {
    Path result(lhs);
    result /= rhs;
    return result;
}

Path operator/(Path&& lhs, const PathView& rhs) {
    lhs /= rhs;
    return std::move(lhs);
}

Path operator+(const PathView& lhs, const PathView& rhs) {
    Path result(lhs);
    result += rhs;
    return result;
}

Path operator+(Path&& lhs, const PathView& rhs) {
    lhs += rhs;
    return std::move(lhs);
}

}
