#include <asp/fs/Path.hpp>
#include <asp/iter.hpp>

namespace asp::fs {

// Shared impl

static auto startsWithOneOf(std::string_view str, std::string_view chars) {
    return !str.empty() && chars.contains(str.front());
}

static auto endsWithOneOf(std::string_view str, std::string_view chars) {
    return !str.empty() && chars.contains(str.back());
}

size_t _impl::hash(std::string_view path) noexcept {
    return std::hash<std::string_view>()(path);
}

std::optional<PathView> _impl::parent(std::string_view path) noexcept {
    auto slash = path.find_last_of(SupportedSep);
    if (slash == std::string::npos) {
        return std::nullopt;
    }

    auto rn = rootName(path);
    if (rn && slash < rn->size()) {
        return *rn;
    }

    // "/" has no parent
    if (slash == 0 && slash + 1 >= path.size()) {
        return std::nullopt;
    }

    // "/foo": strip last component
    if (slash == 0) {
        return PathView(path.substr(0, 1));
    }

    return PathView(path.substr(0, slash));
}

std::optional<std::string_view> _impl::filename(std::string_view path) noexcept {
    auto slash = path.find_last_of(SupportedSep);
    if (slash == std::string::npos && !path.empty()) {
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

    if (dot == 0 && fn->find_first_of('.', 1) == std::string::npos) {
        return std::nullopt;
    }

    return fn->substr(dot + 1);
}

std::optional<std::string_view> _impl::stem(std::string_view path) noexcept {
    auto fn = _impl::filename(path);
    if (!fn) return std::nullopt;

    auto dot = fn->find_last_of('.');
    if (dot == std::string::npos || dot == 0) return *fn;

    return fn->substr(0, dot);
}

bool _impl::equal(std::string_view lhs, std::string_view rhs) noexcept {
    return lhs == rhs;
}

std::strong_ordering _impl::compare(std::string_view lhs, std::string_view rhs) noexcept {
    return lhs <=> rhs;
}

Path _impl::lexicallyNormal(std::string_view path) {
    bool trailing = endsWithOneOf(path, SupportedSep);
    bool absolute = isAbsolute(path);

    std::vector<std::string_view> components;
    bool hasRoot = false;

    for (auto comp : PathComponentIterator { path }) {
        if (comp == ".") continue;
        if (comp == "..") {
            if (!components.empty() && components.back() != ".." && !(hasRoot && components.size() == 1)) {
                components.pop_back();
            } else if (!hasRoot) {
                components.push_back("..");
            }
        } else {
            if (absolute && !hasRoot) {
                hasRoot = true;
            }
            components.push_back(comp);
        }
    }

    Path result;
    for (auto comp : components) {
        result /= comp;
    }

    if (trailing && !result.empty() && !endsWithOneOf(result.view(), SupportedSep)) {
        result += PreferredSep;
    }
    return result;
}

std::optional<Path> _impl::lexicallyRelative(std::string_view path, std::string_view base) {
    auto a = _impl::lexicallyNormal(path);
    auto b = _impl::lexicallyNormal(base);

    if (a.rootName() != b.rootName()) {
        return std::nullopt; // no common root, e.g.
    }

    // compute how many matching components there are
    size_t i = 0;
    for (auto [compA, compB] : a.components().zip(b.components())) {
        if (compA != compB) {
            break;
        }
        i++;
    }

    size_t bComponents = b.components().count();

    Path out;
    for (size_t j = i; j < bComponents; j++) {
        out /= "..";
    }
    for (auto c : a.components().skip(i)) {
        out /= c;
    }

    return out;
}

Result<Path> _impl::weaklyCanonical(std::string_view path) {
    GEODE_UNWRAP_INTO(auto abs, absolute(path).map([](Path p) { return p.lexicallyNormal(); }));
    std::vector<std::string> suffix;

    Path prefix = abs;
    while (!prefix.empty() && !prefix.exists()) {
        if (auto fn = prefix.filename()) {
            suffix.push_back(std::string{*fn});
        }
        if (auto par = prefix.parent()) {
            prefix = *par;
        } else {
            break;
        }
    }

    if (prefix.empty()) {
        return Err("no existing ancestor");
    }

    GEODE_UNWRAP_INTO(auto canon, prefix.canonicalize());
    for (auto& s : asp::iter::fromReverse(suffix)) {
        canon /= s;
    }

    return Ok(canon.lexicallyNormal());
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

    bool startsWithSep = startsWithOneOf(path.view(), SupportedSep);
    if (path.isAbsolute() || startsWithSep) {
        m_path.assign(path);
        return *this;
    }

    if (!m_path.empty() && !endsWithOneOf(m_path, SupportedSep)) {
        m_path += PreferredSep;
    }
    m_path += path.view();

    return *this;
}

Path& Path::operator+=(PathView path) {
    return this->concat(path);
}

Path& Path::operator+=(char c) {
    return this->concat(c);
}

Path& Path::concat(PathView path) {
    m_path += path.view();
    return *this;
}

Path& Path::concat(char c) {
    return this->concat(std::string_view{&c, 1});
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
    if (auto ext = this->extension()) {
        m_path.erase(m_path.size() - ext->size() - 1);
    }

    if (extension.empty()) {
        return *this;
    }

    if (!extension.starts_with('.')) {
        m_path += '.';
    }

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

std::optional<std::string_view> PathComponentIterator::next() noexcept {
    if (m_first) {
        m_first = false;
        if (auto rn = PathView{m_path}.rootName()) {
            m_path.remove_prefix(rn->size());
            return rn;
        }
    }

    while (!m_path.empty()) {
        auto slash = m_path.find_first_of(SupportedSep);
        if (slash == std::string::npos) {
            return std::exchange(m_path, {});
        }

        auto component = m_path.substr(0, slash);
        m_path.remove_prefix(slash + 1);

        if (!component.empty()) {
            return component;
        }
    }

    return std::nullopt;
}

}
