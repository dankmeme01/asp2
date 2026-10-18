# asp2

C++ utility library, with many parts striving to replicate functionality of Rust's standard library.

Some highlights include:
* [`asp::iter`](#iterators) - a feature-rich, extensive iterator implementation that allows chaining various iterator methods
* [`asp::fs`](#filesystem) - lightweight reimplementation of `std::filesystem`, using simple UTF-8 paths, convenient buffered & unbuffered file IO, and more
* `asp::time` - ways to obtain and tinker with system/monotonic time as well as just durations, far more convenient than `std::chrono`
* `asp::SmallVec<T, N>` - a growable container that can store up to N elements inline until falling back to heap allocation
* `asp::Mutex<T>` - a convenient wrapper type that stores a value and provides a way to access it via a RAII guard
* `asp::SpinLock<T>` - same as Mutex but using a spinlock instead
* `asp::Notify` - synchronous notifications aka simpler condition variable
* `asp::Channel<T>` a simple thread-safe message channel

# Iterators

One of the best features of `asp` are Rust-style iterators in the `asp::iter` module, they are *much* easier to implement than C++ iterators and immediately come with a bunch of convenient operations. Here is a simple number-based example:

```cpp
#include <asp/iter.hpp>

// create an iterator representing a range [0; 5)
auto range = asp::iter::range(0, 5);

while (std::optional<int> item = range.next()) {
    std::cout << item << std::endl;
}
std::cout << "iterator was exhausted!" << std::endl;
```

This will print:
```
0
1
2
3
4
iterator was exhausted!
```

This isn't the only way to iterate them too, it is only a convenience addition. C++ range-based for loops are supported too:
```cpp
auto range = asp::iter::range(0, 5);

for (int item : range) {
    std::cout << item << std::endl;
}
// prints the same result
```

Similar to `std::ranges`, they support a *lot* of transforming and terminal operations as well, most common being `map`, `filter`, `join`, `collect`:

```cpp
// Create a range [0, 1, 2, 3, 4]
auto vec = asp::iter::range(0, 5)
    // Multiply every element by 2
    .map([](int n) { return n * 2; })
    // Reject elements >= 6
    .filter([](int n) { return n < 6; })
    // Convert every element to a string
    .map([](int n) { return std::to_string(n); })
    // Collect into a vector of strings
    .collect();

for (const std::string& item : vec) {
    std::cout << item << std::endl;
}

// Using `from`, we can make an asp iterator into any C++ collection
auto str = asp::iter::from(vec)
    // We have an iterator over `std::string&`, join them to a single string
    .join(", ");

std::cout << str << std::endl;
```

This will output:
```
0
2
4
0, 2, 4
```

## Making your own iterator

Making an iterator is as simple as inheriting `asp::iter::Iter`, defining the item type and a `next` function. Here is an example of an iterator that simply generates 3 integers and stops:

```cpp
class ASP_ITER_NODISCARD CustomIter : public asp::iter::Iter<CustomIter, int> {
public:
    using Item = int;

    CustomIter() {}

    std::optional<Item> next() {
        if (m_next >= 3) {
            // nullopt means the iterator is exhausted and expected to not return anything ever again
            return std::nullopt;
        }

        // less than 3 items have been yielded, so yield the next number
        m_next++;
        return m_next;
    }

private:
    int m_next = 0;
};

// example usage:
CustomIter{}
    .map([](int x) { return std::to_string(x * 2); })
    .forEach([](std::string x) {
        std::cout << x << std::endl;
    });
```

Expected output:
```cpp
2
4
6
```

### Size hints

When making a custom iterator, it is useful to add a size hint, when possible. In the example above, we know that the iterator will *always* yield exactly 3 items, and this information can be useful to the caller, so let's add a `sizeHint` method:

```cpp
class ASP_ITER_NODISCARD CustomIter : public asp::iter::Iter<CustomIter, int> {
public:
    // ...

    size_t sizeHint() {
        return 3;
    }

    // ...
};
```

This way, when you call `collect()`, the library will know that it should expect roughly 3 elements to be yielded. This is not a 100% contract but rather a simple hint, it allows calling `.reserve` on a collection before pushing elements to it so that no unnecessary allocations are made to expand the returned container.

Most iterators that return a deterministic amount of elements (such as `map`, `inspect`) override this, and those that can't (e.g. `filter` due to the actual number being runtime-dependent) don't.

### Wrapping another iterator

It can be useful to make your own transformer iterator, similar to `map` or `filter` which consume elements of another iterator to produce their own. This is also relatively straightforward. For example, let's make an iterator that converts elements to a string:

```cpp
/// Declare our iterator as a template, so any other iterator `It` can be used with it
template <typename It>
class ASP_ITER_NODISCARD StringConverter : public asp::iter::Iter<StringConverter<It>, std::string> {
public:
    using Item = std::string;

    StringConverter(It iter) : m_iter(std::move(iter)) {}

    std::optional<Item> next() {
        if (auto item = m_iter.next()) {
            // note: for non-trivially-copiable things, `next` may return a std::reference_wrapper,
            // so typically you want to unwrap it by using `unwrapRefWrapper`
            decltype(auto) value = unwrapRefWrapper(std::move(*item));

            return std::to_string(value);
        }

        // inner iterator ran out of items, nothing to return
        return std::nullopt;
    }

    // only provide a size hint when the underlying collection is able to provide one
    size_t sizeHint() const requires(asp::iter::ProvidesSizeHint<It>) {
        // because we are a simple 1:1 mapping iterator, the hint is the same
        return m_iter.sizeHint();
    }

private:
    It m_iter;
};

// Example usage:
auto elements = StringConverter{range(1, 10)}
    .collect();

// elements is now a vector of strings "1" to "9"
```

## Collectors

By default the following collections support `collect` automatically:
* `std::vector`
* `std::deque`
* `std::queue`
* `std::unordered_set`
* `std::set`
* `std::unordered_map` (requires pairs)
* `std::map` (requires pairs)
* `Result<std::vector<T>, E>` from an iterator of `Result<T, E>` (converts entire iterator into first encountered error, otherwise returns all results)
* `asp::SmallVec<T, N>`

(note: they require the `<asp/iter/Collectors.hpp>` header to be included, if you decide to not include the full `<asp/iter.hpp>`)

For maps, the iterator must yield pairs:
```cpp
std::vector<std::string> items{ "a", "b", "c" };

auto map = asp::iter::from(items)
    .enumerate() // turns it into std::pair<size_t, std::string&>
    .collect<std::map<size_t, std::string>>();

// map is now { 0: "a", 1: "b", 2: "c" }
```

If you need to collect items into a different collection, implementing a custom collector is simple:
```cpp
// say MyVector<T> is your custom collection
template <typename T>
struct asp::iter::Collector<MyVector<T>> {
    Collector(std::optional<size_t> sizeHint) {
        // if your collection supports reserving space, you may use the `sizeHint` argument for it
        if (sizeHint) {
            m_container.reserve(*sizeHint);
        }
    }

    // `push` will be called with each element taken from the iterator
    template <typename Y>
    void push(Y&& item) {
        m_container.push_back(std::forward<Y>(item));
    }

    // `finish` is called at the end and must return the collection
    auto finish() {
        return std::move(m_container);
    }

private:
    MyVector<T> m_container;
};
```

## Misc

The iterator module comes with a few nice shorthands to make your life easier too, such as:
* `asp::iter::fromReverse` -> produces a reverse iterator over a collection
* `asp::iter::consume` -> takes a container and moves elements out of it instead of yielding references like `from`
* `asp::iter::contains` -> shorthand for `std::find(cont.begin(), cont.end(), item)`
* `asp::iter::keys` -> returns an iterator over map keys
* `asp::iter::values` -> returns an iterator over map keys
* `asp::iter::lines` -> returns an iterator over substrings, splitting by `\n` or `\r\n` newlines
* `asp::iter::repeat` -> returns an iterator that repeats the same item multiple times
* `asp::iter::split / splitAny / splitWhitespace` -> return an iterator over substrings in a string, splitting by given delimiter
* All the other methods inside `Iter`: `arrayChunks`, `inspect`, `filterMap`, `flatMap`, `fold`, `chain`, `take`, `skip`, `cycle`, `min`, `sum`, `max`, `all`, `any`, etc...

## Performance

Every single asp iterator is *lazy* and typically non allocating by itself. This includes mapping, splitting, etc. For example this code:

```cpp
(void) asp::iter::range(0, 100000000)
    .map([](int x) { return x * 10000; })
    .filter([](int x) { return x < rand(); })
    .map([](int x) { return std::to_string(x); });
```

will use exactly 0 cpu cycles and bytes of memory! For iterators to actually do work, you must manually drive progress by calling `next()` or calling a *terminal* operation that consumes the whole iterator, such as `collect`, `count`, `min`, `max`, `sum`, etc.

Every item is processed one-by-one when `next` is called, there is no cost of pre-computing anything. This applies to `split` functions too:

```cpp
for (auto s : asp::iter::splitWhitespace("hi this is a string")) {
    std::cout << s << std::endl;
}
```

This does not scan the whole string at once, instead it scans until the next space and moves the cursor, again and again as the loop goes. This makes `asp::iter` iterators a zero-cost abstraction that's just as fast as manual loops, but without the extra boilerplate.

# Filesystem

`asp::fs` module provides various path and IO operations, aims to be similar to Rust's `std::fs`/`std::path` and in some aspects C++ `std::filesystem`, while staying lightweight and having a platform independent API with little surprises.

## Path and PathView

Analogous to `std::string` and `std::string_view`, these are the two primary classes for representing filesystem paths. Unlike `std::filesystem::path`, they have the same representation on all platforms (typically UTF-8, however not always guaranteed), so methods like `.string()` or `.view()` are zero-cost and perform no conversions.

Here are some examples of what you can do with paths:

```cpp
// obtain path to the temporary directory, such as /tmp/
// may not work on all systems, so can return an error
Path temp = Path::tmp().unwrap();

auto myFile = temp / "test.txt";

// let's read the file contents if it already exists
if (myFile.exists()) {
    if (myFile.isFile()) {
        auto result = asp::fs::readToString(myFile);
        if (!result) {
            std::cout << "failed to read: " << result.unwrapErr() << std::endl;
        } else {
            std::cout << result.unwrap() << std::endl;
        }
    } else {
        std::cout << "test.txt exists but it's not a file !!!" << std::endl;
    }
}

// now let's overwrite the file
auto result = asp::fs::write(myFile, "new contents");
if (!result) {
    std::cout << "failed to write: " << result.unwrapErr() << std::endl;
}

// some other path operations:
Path p = myFile.parent(); // will return the same path as `temp`
assert(p.equivalent(temp));

Path asExe = myFile.withExtension(".exe");
assert(asExe.filename() == "test.exe");


// Let's assume that our current directory (cwd) is the same as temp dir,
// so this relative path points to the same .txt file we created:
Path rel{"test.txt"};
// Because one is "test.txt" and another is "/tmp/test.txt" (or similar), the equality check will fail
assert(rel != myFile);
// However, the equivalence check doesn't care about path strings, so it will succeed:
assert(rel.equivalent(myFile));
// And we can expect the canonical representation of the relative path to also be /tmp/test.txt:
assert(rel.canonicalize().unwrap() == myFile);
```

## File

`File` is a class similar to `std::fstream`, with nicer Result-based API and **no buffering**, every read/write call invokes a syscall. While `asp::fs::read`/`asp::fs::write` functions exist as simpler shorthands, `File` can be used when richer api is needed:

```cpp
// let's create a new file, the `create` shorthand will truncate the file for us if it already exists
{
    auto file = asp::fs::File::create("/tmp/test.txt").unwrap();
    file.write("new contents").unwrap();
    // no need to manually close the file, it will be done during destruction of File
}

// now let's read it back
{
    auto file = asp::fs::File::open("/tmp/test.txt").unwrap();
    std::string contents = file.readToString().unwrap();
    std::cout << contents << std::endl;
}
```

`File` features other member functions that could be useful for seeking, cloning, checking metadata, locking/unlocking, or syncing data to disk.

### BufferedFile

If you want to read/write from/to a file incrementally, it is highly advised to use `BufferedFile`, which will buffer data internally and only invoke syscalls when necessary. `File` has a shorthand to create a buffered file:

```cpp
// openBuffered or createBuffered will open a file and wrap it into a `BufferedFile`
auto file = asp::fs::File::openBuffered("/tmp/test.txt").unwrap();

// now we can even read 1 byte at a time and it stays efficient!
uint8_t byte;

while (file.read(std::span{&byte, 1}).isOk()) {
    // ...
}
```
