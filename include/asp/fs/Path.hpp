#pragma once

#include <asp/iter/Iter.hpp>
#include "Error.hpp"
#include "Metadata.hpp"

namespace asp::fs {

#ifdef _WIN32
using NativeChar = wchar_t;
constexpr inline char PreferredSep = '\\';
constexpr inline auto SupportedSep = "/\\";
#else
using NativeChar = char;
constexpr inline char PreferredSep = '/';
constexpr inline auto SupportedSep = "/";
#endif

using NativePath = std::basic_string<NativeChar>;

class Path;
class PathView;

namespace _impl {
    Result<NativePath> native(std::string_view path);
    size_t hash(std::string_view path) noexcept;
    bool isAbsolute(std::string_view path) noexcept;

    std::optional<PathView> parent(std::string_view path) noexcept;
    std::optional<std::string_view> filename(std::string_view path) noexcept;
    std::optional<std::string_view> extension(std::string_view path) noexcept;
    std::optional<std::string_view> stem(std::string_view path) noexcept;
    std::optional<std::string_view> rootName(std::string_view path) noexcept;

    Result<Path> canonicalize(std::string_view path);
    Result<Path> absolute(std::string_view path);
    Path lexicallyNormal(std::string_view path);
    std::optional<Path> lexicallyRelative(std::string_view path, std::string_view base);
    Result<Path> weaklyCanonical(std::string_view path);
    Result<Path> readSymlink(std::string_view path);

    bool equal(std::string_view lhs, std::string_view rhs) noexcept;
    bool equivalent(std::string_view lhs, std::string_view rhs, bool followSymlink) noexcept;
    std::strong_ordering compare(std::string_view lhs, std::string_view rhs) noexcept;

    bool exists(std::string_view path) noexcept;
    Result<Metadata> metadata(std::string_view path, bool followSymlink);
}

template <typename Derived>
struct PathInterface {
    /// Returns a representation of this path that is suitable for use with OS APIs.
    /// On Windows, this converts the path to UTF-16 and returns a wide string.
    /// On other systems, this simply copies the path and is equivalent to `string()`.
    /// If the conversion fails or the path is empty, returns an empty string.
    NativePath native(this auto&& self) {
        return _impl::native(self.m_path).unwrapOrDefault();
    }

    // Like `native`, but returning a Result instead, indicating errors if the conversion fails
    Result<NativePath> nativeResult(this auto&& self) {
        return _impl::native(self.m_path);
    }

    /// Converts this path to a `std::filesystem::path`. This may fail if the UTF-16 conversion fails on Windows.
    /// This requires <asp/fs/std.hpp> to be included.
    auto toStdPath(this auto&& self);

    bool empty(this auto&& self) noexcept {
        return self.m_path.empty();
    }

    size_t hash(this auto&& self) noexcept {
        return _impl::hash(self.m_path);
    }

    /// Returns the size of the current path representation, in bytes, equivalent to `string().size()`
    size_t size(this auto&& self) noexcept {
        return self.m_path.size();
    }

    bool isAbsolute(this auto&& self) noexcept {
        return _impl::isAbsolute(self.m_path);
    }

    bool isRelative(this auto&& self) noexcept {
        return !_impl::isAbsolute(self.m_path);
    }

    /// Appends the given path to this path, introducing a directory separator between two parts.
    /// Same as `Path{*this} /= other`.
    Path join(this auto&& self, PathView other);


    // -- decomposition -- //


    /// Returns a lexical parent path of this path, or `std::nullopt` if this path has no parent.
    /// This performs no filesystem operations, and for example `/foo/.` will return `/foo` as parent instead of `/`.
    /// If the path is a root path (such as `/` or `C:/`), this will return the same path, aka `rootName()`.
    std::optional<PathView> parent(this auto&& self) noexcept;

    /// Returns whether this path has a parent path. This is equivalent to `parent().has_value()`.
    inline bool hasParent(this auto&& self) noexcept {
        return self.parent().has_value();
    }

    /// Returns the filename component of this path, or `std::nullopt` if this path has no filename.
    std::optional<PathView> filename(this auto&& self) noexcept;

    /// Returns whether the path has a filename. This is equivalent to `filename().has_value()`.
    inline bool hasFilename(this auto&& self) noexcept {
        return self.filename().has_value();
    }

    /// Returns the extension of the filename, or `std::nullopt` if the filename has no extension.
    /// The extension is defined as the substring after the last dot in the filename, and does not include the dot itself.
    /// If the filename ends with a dot, returns an empty view.
    /// If the filename starts with a dot and has no other dots, returns `std::nullopt`.
    std::optional<std::string_view> extension(this auto&& self) noexcept;

    /// Returns whether the filename has an extension. This is equivalent to `extension().has_value()`.
    inline bool hasExtension(this auto&& self) noexcept {
        return self.extension().has_value();
    }

    /// Returns the path with extension replaced via `replaceExtension`
    Path withExtension(this auto&& self, std::string_view newExt) noexcept;

    /// Returns the stem of the path, which is the filename without the extension. If the filename has no '.' or begins with a '.' (with no other '.'), returns the entire filename.
    /// If there is no file name, returns `std::nullopt`.
    std::optional<std::string_view> stem(this auto&& self) noexcept;

    /// Returns whether the path has a stem. This is equivalent to `stem().has_value()`.
    inline bool hasStem(this auto&& self) noexcept {
        return self.stem().has_value();
    }

    /// Returns the root name of the path, or `std::nullopt` if the path has no root name.
    /// On Windows, this returns "C:" or "C:/", on other platforms, this returns `/`. For relative paths, returns `std::nullopt`.
    std::optional<std::string_view> rootName(this auto&& self) noexcept;

    /// Returns whether the path has a root name. This is equivalent to `rootName().has_value()`.
    inline bool hasRootName(this auto&& self) noexcept {
        return self.rootName().has_value();
    }

    /// Returns the canonical, absolute form of the path, resolving all symlinks and normalizing relative components.
    /// This will error if the path does not exist, or any intermediate components are not directories.
    Result<Path> canonicalize(this auto&& self);

    /// Returns an absolute form of the path. If this path is relative, it is resolved against the current working directory.
    /// Unlike `canonicalize`, this does not resolve symlinks and may or may not resolve other intermediate components.
    /// This may or may not error if the path does not exist, depending on the platform.
    Result<Path> absolute(this auto&& self);

    /// Returns the lexically normal form of the path, resolving components like `.` and `..`.
    /// This performs no fs operations, so may produce undesired results when symlinks are used.
    Path lexicallyNormal(this auto&& self);

    /// Returns the lexically relative form of the path, relative to the given base path. If the path is not relative to the base, returns `std::nullopt`.
    /// This performs no fs operations, and simply calls `lexicallyNormal` on both paths and checks if they share a common prefix.
    std::optional<Path> lexicallyRelative(this auto&& self, PathView base);

    /// Returns the lexically proximate form of the path, similar to `lexicallyRelative`,
    /// but it simply returns the input path if `lexicallyRelative` returns `std::nullopt`.
    Path lexicallyProximate(this auto&& self, PathView base);

    /// Like `canonicalize`, but does not have to fail if the path does not exist.
    /// This will resolve all possible symlinks and relative components on the way,
    /// but once a non-existing component is reached, the remainder of the path is appended as-is.
    Result<Path> weaklyCanonical(this auto&& self);

    // -- comparison -- //

    /// Compares two paths. This is a simple string comparison, and does not take into account any filesystem semantics.
    std::strong_ordering operator<=>(this const Derived& self, const Derived& other) noexcept;

    /// Checks if two paths are equivalent and point to the same file in the filesystem.
    /// If either does not exist, or any other error occurred, returns false.
    ///
    /// Note that this follows all symlinks, including the target one.
    /// So if this path refers to a specific file, and `other` is a symlink to the same file (or vice versa), this will return `true`.
    bool equivalent(this const Derived& self, PathView other) noexcept;

    /// Similar to `equivalent`, checks if two files point to the same in the filesystem, but does not follow the final link.
    /// This means two links pointing to the same file may return `false`.
    bool symlinkEquivalent(this const Derived& self, PathView other) noexcept;

    // -- fs operations -- //


    /// Checks whether this path exists in the filesystem. If for whatever reason the check fails, such as lacking permissions, returns `false`.
    bool exists(this auto&& self) noexcept {
        return _impl::exists(self.m_path);
    }

    /// Obtains the metadata of the file or directory at this path. Returns an error in the following situations (but not limited to):
    /// * Path does not exist
    /// * Path is a symlink and the target does not exist
    /// * Lack of permissions
    Result<Metadata> metadata(this auto&& self) {
        return _impl::metadata(self.m_path, true);
    }

    /// Obtains the metadata of the file or symlink at this path. If the final component is a symlink, unlike `metadata`,
    /// this does NOT follow the symlink and instead returns metadata of the actual final symlink.
    /// Returns an error in the following situations (but not limited to):
    /// * Path does not exist (but symlink target may or may not exist)
    /// * Lack of permissions
    Result<Metadata> symlinkMetadata(this auto&& self) {
        return _impl::metadata(self.m_path, false);
    }

    /// Checks whether this path exists and is a regular file. Returns `false` on failures. For detailed error information, call `metadata()`.
    /// This will resolve intermediate symlinks, but NOT the final symlink, and will return false if the final component is a symlink.
    bool isFile(this auto&& self) {
        return self.symlinkMetadata().map([](auto&& meta) {
            return meta.isFile();
        }).unwrapOr(false);
    }

    /// Checks whether this path exists and is a directory. Returns `false` on failures. For detailed error information, call `metadata()`.
    /// This will resolve intermediate symlinks, but NOT the final symlink, and will return false if the final component is a symlink.
    bool isDir(this auto&& self) {
        return self.symlinkMetadata().map([](auto&& meta) {
            return meta.isDir();
        }).unwrapOr(false);
    }

    /// Checks whether this path exists and is a symlink. Returns `false` on failures. For detailed error information, call `metadata()`.
    bool isSymlink(this auto&& self) {
        return self.symlinkMetadata().map([](auto&& meta) {
            return meta.isSymlink();
        }).unwrapOr(false);
    }

    /// Attempts to read the symlink and return the target path, fails if the path is not a symlink.
    Result<Path> readSymlink(this auto&& self);

    // -- iteration -- //

    /// Returns an `asp::iter` iterator over the components of this path. This performs no fs operations and performs only a few normalization steps:
    /// * Empty components are excluded, e.g. `a//b`, `a/b` and `a/b/` all yield `["a", "b"]`
    /// * Absolute path contains `rootName()` as the first component. This means, `/foo/bar` yields `["/", "foo", "bar"]`,
    /// and `C:/foo/bar` yields `["C:/", "foo", "bar"]` on Windows.
    auto components(this auto&& self) noexcept;

    /// Returns whether the first component of this path is equal to the given prefix.
    /// This is NOT equivalent to a string `starts_with` check, it checks components specifically.
    /// For example:
    /// * `"/foo/bar".startsWith("/")` is true
    /// * `"/foo/bar".startsWith("/foo")` is false
    /// * `"/foo/bar".startsWith("foo")` is false
    /// * `"foo/bar".startsWith("foo")` is true
    /// * `"foo/bar".startsWith("foo/")` is false
    bool startsWith(this auto&& self, std::string_view prefix) noexcept {
        auto first = self.components().next();
        return first && *first == prefix;
    }

    /// Returns whether the last component of this path is equal to the given suffix.
    /// This is NOT equivalent to a string `ends_with` check, it checks components specifically.
    bool endsWith(this auto&& self, std::string_view suffix) noexcept {
        auto last = self.components().last();
        return last && *last == suffix;
    }
};

class PathView : public PathInterface<PathView> {
public:
    PathView() = default;
    PathView(std::string_view path) : m_path(path) {}
    PathView(const std::string& path) : m_path(path) {}
    PathView(const char* path) : m_path(path) {}

    PathView(const PathView&) noexcept = default;
    PathView(PathView&&) noexcept = default;

    /// Returns the string representation of the path. See `Path::string` for encoding details.
    operator std::string_view() const noexcept;

    /// Returns the string representation of the path. See `Path::string` for encoding details.
    std::string_view view() const noexcept;
    /// Returns the string representation of the path. See `Path::string` for encoding details.
    std::string string() const;

    /// Allocates a new, owned `Path` referencing the same path as this view.
    Path toPath() const;

private:
    friend struct PathInterface<PathView>;
    std::string_view m_path;
};

class Path : public PathInterface<Path> {
public:
    Path() = default;
    Path(PathView path) : m_path(path) {}
    Path(const char* path) : m_path(path) {}

    template <typename T> requires std::same_as<std::decay_t<T>, std::string>
    Path(T&& path) : m_path(std::forward<T>(path)) {}

#ifdef _WIN32
    /// Converts the given UTF-16 wide string to a UTF-8 path, may fail if conversion fails.
    static Result<Path> fromWideResult(std::wstring_view path);

    /// Converts the given UTF-16 wide string to a UTF-8 path, returns empty path if conversion fails.
    static Path fromWide(std::wstring_view path) {
        return fromWideResult(path).unwrapOrDefault();
    }
#endif

    static Path fromStdPath(const auto& stdPath);

    Path(const Path&) = default;
    Path(Path&&) noexcept = default;
    Path& operator=(const Path&) = default;
    Path& operator=(Path&&) noexcept = default;

    Path& assign(PathView path);
    Path& assign(std::string&& path) noexcept;

    template <typename It>
    Path& assign(It begin, It end) {
        m_path.assign(begin, end);
        return *this;
    }

    /// Returns the current working directory. May error if the current directory does not exist, or the user has insufficient permissions.
    static Result<Path> cwd();
    /// Returns the user home directory. May error if the directory does not exist, or the user has insufficient permissions.
    /// On Unix, this is equivalent to the `$HOME` environment variable, also known as `~`.
    /// On Windows, this is equivalent to the `%USERPROFILE%` environment variable.
    static Result<Path> home();
    /// Returns the temporary directory.
    /// On Windows, this will try `TMP`, `TEMP` and `USERPROFILE` environment variables, as done by `GetTempPath`.
    /// On Unix this will try to return the value of the `TMPDIR` environment variable, and if it's unset, then the value is OS specific:
    /// * On Android, this will return `/data/local/tmp`.
    /// * On Apple platforms, this returns `confstr(_CS_DARWIN_USER_TEMP_DIR, ...)`.
    /// * On all other platforms, this will return `/tmp`.
    static Result<Path> tmp();

    /// Appends a component to the path, introducing a directory separator between two parts.
    /// If the given path is an absolute path or a path starting with a directory separator, it replaces the current path instead of appending.
    Path& operator/=(PathView path);

    /// Appends a component to the path, introducing a directory separator between two parts.
    /// If the given path is an absolute path or a path starting with a directory separator, it replaces the current path instead of appending.
    Path& append(PathView path);

    /// Concatenates a component to the path, without introducing a directory separator.
    /// For example, "a" + "b" would become "ab" and not "a/b"
    Path& operator+=(PathView path);

    /// Concatenates a character to the path, without introducing a directory separator.
    Path& operator+=(char c);

    /// Concatenates a component to the path, without introducing a directory separator.
    /// For example, "a" + "b" would become "ab" and not "a/b"
    Path& concat(PathView path);

    /// Concatenates a character to the path, without introducing a directory separator.
    Path& concat(char c);

    /// Clears the path, making it empty.
    void clear() noexcept;

    /// Reserves extra capacity in the storage of the path, to avoid reallocations when appending.
    void reserve(size_t size);

    /// Shrinks the capacity of the path to fit its size, freeing any extra memory.
    void shrinkToFit();

    /// Converts all directory separators in the path to the preferred separator for the current platform.
    /// On Windows, this will convert all forward slashes to backslashes. On other platforms, does nothing.
    Path& makePreferred() noexcept;

    /// Removes the filename component from the path, leaving only the parent path.
    Path& removeFilename();

    /// Replaces the filename component of the path with the given filename.
    /// This is equivalent to calling `removeFilename()` followed by `operator/=`.
    Path& replaceFilename(std::string_view filename);

    /// Replaces the extension of the filename with the given extension. If no extension is present, it is added.
    /// If the given extension begins with a dot, that dot is ignored. That means, `.txt` and `txt` are equivalent, but not `..txt`.
    /// If the filename ends with a single dot, it is stripped before replacement.
    ///
    /// Examples:
    /// * `file.txt` with `md` -> `file.md`
    /// * `file.txt` with `.md` -> `file.md`
    /// * `file.txt` with `..md` -> `file..md`
    /// * `file` with `md` -> `file.md`
    /// * `file` with `..md` -> `file..md`
    /// * `file.` with `md` -> `file.md`
    /// * `file..` with `md` -> `file..md`
    Path& replaceExtension(std::string_view extension);


    // -- getting other representations -- //


    /// Returns the string representation of the path.
    /// This makes no conversions or normalizations, and is a simple string copy.
    /// The returned string is not guaranteed to be of any particular encoding, but paths coming from system APIs are:
    /// * On Windows, guaranteed to be valid UTF-8
    /// * On POSIX, usually UTF-8, but not guaranteed
    std::string string() const&;
    std::string string() &&;

    /// Returns a `PathView` of this path, which is a non-owning view of the path string.
    PathView view() const noexcept;

    /// Returns a `const char*` representation of this path. See notes on `string()` for encoding.
    const char* c_str() const noexcept;

    operator PathView() const noexcept;

private:
    friend struct PathInterface<Path>;
    std::string m_path;
};

template <typename Y>
inline Path operator/(const PathView& lhs, Y&& rhs) {
    Path result{lhs};

    if constexpr (std::is_convertible_v<Y, PathView>) {
        result /= PathView{std::forward<Y>(rhs)};
    } else if constexpr (std::is_convertible_v<Y, std::string_view>) {
        result /= PathView{std::string_view{std::forward<Y>(rhs)}};
    } else {
        static_assert(!std::is_same_v<Y, Y>, "rhs must be convertible to PathView");
    }
    return result;
}

template <typename Y>
inline Path operator/(Path&& result, Y&& rhs) {
    if constexpr (std::is_convertible_v<Y, PathView>) {
        result /= PathView{std::forward<Y>(rhs)};
    } else if constexpr (std::is_convertible_v<Y, std::string_view>) {
        result /= PathView{std::string_view{std::forward<Y>(rhs)}};
    } else {
        static_assert(!std::is_same_v<Y, Y>, "rhs must be convertible to PathView");
    }
    return result;
}

template <typename Y>
inline Path operator+(const PathView& lhs, Y&& rhs) {
    Path result{lhs};

    if constexpr (std::is_convertible_v<Y, PathView>) {
        result += PathView{std::forward<Y>(rhs)};
    } else if constexpr (std::is_convertible_v<Y, std::string_view>) {
        result += PathView{std::string_view{std::forward<Y>(rhs)}};
    } else {
        static_assert(!std::is_same_v<Y, Y>, "rhs must be convertible to PathView");
    }
    return result;
}

template <typename Y>
inline Path operator+(Path&& result, Y&& rhs) {
    if constexpr (std::is_convertible_v<Y, PathView>) {
        result += PathView{std::forward<Y>(rhs)};
    } else if constexpr (std::is_convertible_v<Y, std::string_view>) {
        result += PathView{std::string_view{std::forward<Y>(rhs)}};
    } else {
        static_assert(!std::is_same_v<Y, Y>, "rhs must be convertible to PathView");
    }
    return result;
}

inline bool operator==(const PathView& lhs, const PathView& rhs) noexcept {
    return _impl::equal(lhs.view(), rhs.view());
}

inline std::strong_ordering operator<=>(const Path& lhs, const PathView& rhs) noexcept {
    return lhs.view() <=> rhs;
}

inline std::strong_ordering operator<=>(const PathView& lhs, const Path& rhs) noexcept {
    return lhs <=> rhs.view();
}

inline auto format_as(const Path& path) {
    return fmt::format("{}", path.view().view());
}

inline auto format_as(const PathView& path) {
    return fmt::format("{}", path.view());
}

// component iterator

struct PathComponentIterator : public iter::Iter<PathComponentIterator, std::string_view> {
    PathComponentIterator(std::string_view path) : m_path(path) {}
    std::optional<Item> next() noexcept;

private:
    std::string_view m_path;
    bool m_first = true;
};

// out of line definitions

template <typename D>
Path PathInterface<D>::join(this auto&& self, PathView other) {
    return Path{self} /= other;
}

template <typename D>
std::optional<PathView> PathInterface<D>::parent(this auto&& self) noexcept {
    return _impl::parent(self.m_path);
}

template <typename D>
std::optional<PathView> PathInterface<D>::filename(this auto&& self) noexcept {
    return _impl::filename(self.m_path);
}

template <typename D>
std::optional<std::string_view> PathInterface<D>::extension(this auto&& self) noexcept {
    return _impl::extension(self.m_path);
}

template <typename D>
Path PathInterface<D>::withExtension(this auto&& self, std::string_view newExt) noexcept {
    Path path{self.m_path};
    path.replaceExtension(newExt);
    return path;
}

template <typename D>
std::optional<std::string_view> PathInterface<D>::stem(this auto&& self) noexcept {
    return _impl::stem(self.m_path);
}

template <typename D>
std::optional<std::string_view> PathInterface<D>::rootName(this auto&& self) noexcept {
    return _impl::rootName(self.m_path);
}

template <typename D>
Result<Path> PathInterface<D>::canonicalize(this auto&& self) {
    return _impl::canonicalize(self.m_path);
}

template <typename D>
Result<Path> PathInterface<D>::absolute(this auto&& self) {
    return _impl::absolute(self.m_path);
}

template <typename D>
Path PathInterface<D>::lexicallyNormal(this auto&& self) {
    return _impl::lexicallyNormal(self.m_path);
}

template <typename D>
std::optional<Path> PathInterface<D>::lexicallyRelative(this auto&& self, PathView base) {
    return _impl::lexicallyRelative(self.m_path, base.view());
}

template <typename D>
Path PathInterface<D>::lexicallyProximate(this auto&& self, PathView base) {
    return self.lexicallyRelative(base).value_or(Path{self});
}

template <typename D>
Result<Path> PathInterface<D>::weaklyCanonical(this auto&& self) {
    return _impl::weaklyCanonical(self.m_path);
}

template <typename D>
std::strong_ordering PathInterface<D>::operator<=>(this const D& self, const D& other) noexcept {
    return _impl::compare(self.m_path, other.m_path);
}

template <typename D>
bool PathInterface<D>::equivalent(this const D& self, PathView other) noexcept {
    return _impl::equivalent(self.m_path, other.view(), true);
}

template <typename D>
bool PathInterface<D>::symlinkEquivalent(this const D& self, PathView other) noexcept {
    return _impl::equivalent(self.m_path, other.view(), false);
}

template <typename D>
Result<Path> PathInterface<D>::readSymlink(this auto&& self) {
    return _impl::readSymlink(self.m_path);
}

template <typename D>
auto PathInterface<D>::components(this auto&& self) noexcept {
    return PathComponentIterator{std::string_view{self.m_path}};
}

}

namespace std {
    template <>
    struct hash<asp::fs::Path> {
        size_t operator()(const asp::fs::Path& path) const noexcept {
            return path.hash();
        }
    };

    template <>
    struct hash<asp::fs::PathView> {
        size_t operator()(const asp::fs::PathView& path) const noexcept {
            return path.hash();
        }
    };
}
