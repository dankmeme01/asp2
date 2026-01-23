# asp2

C++ utility library, with many parts striving to replicate functionality of Rust's standard library.

Some highlights include:
* `asp::iter` - a feature-rich, extensive iterator implementation that allows chaining various iterator methods
* `asp::time` - ways to obtain and tinker with system/monotonic time as well as just durations, far more convenient than `std::chrono`
* `asp::SmallVec<T, N>` - a growable container that can store up to N elements inline until falling back to heap allocation
* `asp::fs` - convenient Result-like wrappers around `std::filesystem`
* `asp::Mutex<T>` - a convenient wrapper type that stores a value and provides a way to access it via a RAII guard
* `asp::SpinLock<T>` - same as Mutex but using a spinlock instead
* `asp::Notify` - synchronous notifications aka simpler condition variable
* `asp::Channel<T>` a simple thread-safe message channel