#include <asp/iter.hpp>
#include <gtest/gtest.h>
#include <unordered_set>
#include <array>
#include <cmath>

using namespace asp::iter;
using namespace asp;

static constexpr auto STRING_1 = "This is a long string 1 to bypass SSO";
static constexpr auto STRING_2 = "This is a long string 2 to bypass SSO";
static constexpr auto STRING_3 = "This is a long string 3 to bypass SSO";

TEST(IterTests, VectorIter) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec);

    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), 4);
    EXPECT_EQ(iter.next(), 5);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, VectorIterNTCopiy) {
    std::vector<std::string> vec = {"a", "b", "c", "d", "e"};
    auto iter = from(vec);

    EXPECT_EQ(iter.next()->get(), "a");
    EXPECT_EQ(iter.next()->get(), "b");
    EXPECT_EQ(iter.next()->get(), "c");
    EXPECT_EQ(iter.next()->get(), "d");
    EXPECT_EQ(iter.next()->get(), "e");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Map) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec).map([](int x) { return x * 2; });

    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 4);
    EXPECT_EQ(iter.next(), 6);
    EXPECT_EQ(iter.next(), 8);
    EXPECT_EQ(iter.next(), 10);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, MapComplex) {
    std::vector<std::string> vec = {"1", "22"};
    auto iter = from(vec).map([](std::string& x) { return x.size(); });

    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Filter) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec).filter([](int x) { return x % 2 == 0; });

    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 4);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FilterComplex) {
    std::vector<std::string> vec = {"1", "22"};
    auto iter = from(vec).filter([](std::string& x) { return x.size() == 1; });

    EXPECT_EQ(iter.next()->get(), "1");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FilterMap) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec).filterMap([](int x) -> std::optional<int> {
        if (x % 2 == 0) {
            return x * 2;
        } else {
            return std::nullopt;
        }
    });

    EXPECT_EQ(iter.next(), 4);
    EXPECT_EQ(iter.next(), 8);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FilterMapComplex) {
    std::vector<std::string> vec = {"1", "22", "333", "4444"};
    auto iter = from(vec).filterMap([](std::string& x) -> std::optional<int> {
        if (x.size() % 2 == 0) {
            return x.size();
        } else {
            return std::nullopt;
        }
    });

    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 4);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Chain) {
    std::vector<int> vec1 = {1, 2, 3};
    std::vector<int> vec2 = {6, 7, 8};

    auto iter = from(vec1).chain(from(vec2));

    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), 6);
    EXPECT_EQ(iter.next(), 7);
    EXPECT_EQ(iter.next(), 8);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Take) {
    std::vector<int> vec1 = {1, 2, 3, 4, 5};

    auto iter = from(vec1).take(3);

    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Zip) {
    std::vector<int> vec1 = {1, 2, 3};
    std::vector<double> vec2 = {1.0, 2.0, 3.0};

    auto iter = from(vec1).zip(from(vec2));

    EXPECT_EQ(iter.next(), std::make_pair(1, 1.0));
    EXPECT_EQ(iter.next(), std::make_pair(2, 2.0));
    EXPECT_EQ(iter.next(), std::make_pair(3, 3.0));
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, ZipRefUnwrap) {
    std::vector<std::string> vec1 = {"a", "b", "c"};
    std::vector<std::string> vec2 = {"x", "y", "z"};

    std::string out;
    for (auto [first, second] : from(vec1).zip(from(vec2))) {
        static_assert(std::is_same_v<decltype(first), std::string&>);
        static_assert(std::is_same_v<decltype(second), std::string&>);

        out.append(first);
        out.append(second);
    }

    EXPECT_EQ(out, "axbycz");
}

TEST(IterTests, All) {
    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::vector<int> vec2 = {1, 2, 3, 4, 5, -4};

    EXPECT_TRUE(from(vec1).all([](int x) { return x > 0; }));
    EXPECT_FALSE(from(vec2).all([](int x) { return x > 0; }));
}

TEST(IterTests, Any) {
    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::vector<int> vec2 = {1, 2, 3, 4, 5, -4};

    EXPECT_FALSE(from(vec1).any([](int x) { return x < 0; }));
    EXPECT_TRUE(from(vec2).any([](int x) { return x < 0; }));
}

TEST(IterTests, Find) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};

    EXPECT_EQ(from(vec1).find([](int x) { return x < 0; }), -4);
    EXPECT_EQ(from(vec1).find([](int x) { return x > 5 ; }), std::nullopt);
}

TEST(IterTests, FindStr) {
    std::vector<std::string> vec1 = {"1", "22", "333", "4444", "55555"};

    EXPECT_EQ(from(vec1).find([](const std::string& x) { return x.size() < 3; })->get(), "1");
    EXPECT_EQ(from(vec1).find([](const std::string& x) { return x.size() > 3; })->get(), "4444");
    EXPECT_EQ(from(vec1).find([](const std::string& x) { return x.size() > 5; }), std::nullopt);
}


TEST(IterTests, Enumerate) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto iter = from(vec1).enumerate().collect<std::vector<std::pair<size_t, int>>>();

    EXPECT_EQ(iter, (std::vector<std::pair<size_t, int>>{{0, 1}, {1, 2}, {2, 3}, {3, -4}, {4, 5}}));
}

TEST(IterTests, ForEach) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    int sum = 0;

    from(vec1).forEach([&sum](int x) { sum += x; });
    EXPECT_EQ(sum, 7);
}

TEST(IterTests, Inspect) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    int sum = 0;

    from(vec1).inspect([&sum](int x) { sum += x; }).count();
    EXPECT_EQ(sum, 7);
}

TEST(IterTests, Min) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    EXPECT_EQ(from(vec1).min(), -4);
}

TEST(IterTests, Max) {
    std::vector<int> vec1 = {-1, 2, 333, -4, -49, 5, -100, 0};
    EXPECT_EQ(from(vec1).max(), 333);
}

TEST(IterTests, Sum) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    EXPECT_EQ(from(vec1).sum(), 7);
}

TEST(IterTests, IterMix) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    std::vector<float> vec2 = {-1.5, -43, 3.14, 2.71, -0.42};
    std::vector<char> vec3 = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};

    std::string outstr;
    int outnum = 0;

    size_t count = from(vec1)
        .chain(from(vec2).map([](float x) { return static_cast<int>(roundf(x * x * x)); }))
        .zip(from(vec3))
        .enumerate()
        .filter([](const auto& p) { return p.first % 2 == 0; })
        .map([](const auto& p) { return p.second; })
        .skip(2)
        .inspect([&outstr, &outnum](const auto& p) {
            outstr += p.second;
            outnum += static_cast<float>(p.first);
        })
        .count();

    EXPECT_EQ(outnum, -79482);
    EXPECT_EQ(outstr, "egi");
}

TEST(IterTests, SplitStrChar) {
    std::string str = "hello,world,this,is,a,test";
    auto iter = split(str, ',');

    EXPECT_EQ(iter.next(), "hello");
    EXPECT_EQ(iter.next(), "world");
    EXPECT_EQ(iter.next(), "this");
    EXPECT_EQ(iter.next(), "is");
    EXPECT_EQ(iter.next(), "a");
    EXPECT_EQ(iter.next(), "test");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, SplitStrStr) {
    std::string str = "hello||world||this||is||a||test";
    auto iter = split(str, "||");

    EXPECT_EQ(iter.next(), "hello");
    EXPECT_EQ(iter.next(), "world");
    EXPECT_EQ(iter.next(), "this");
    EXPECT_EQ(iter.next(), "is");
    EXPECT_EQ(iter.next(), "a");
    EXPECT_EQ(iter.next(), "test");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, SplitIntVec) {
    std::vector<unsigned char> vec = {1, 0, 2, 0, 3, 0, 4, 0, 5};
    auto iter = split(vec, 0);

    EXPECT_EQ(iter.next().value()[0], 1);
    EXPECT_EQ(iter.next().value()[0], 2);
    EXPECT_EQ(iter.next().value()[0], 3);
    EXPECT_EQ(iter.next().value()[0], 4);
    EXPECT_EQ(iter.next().value()[0], 5);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, SplitAny) {
    auto iter = splitAny("1,2:3;;4|5-6", ",:;|");

    EXPECT_EQ(iter.next(), "1");
    EXPECT_EQ(iter.next(), "2");
    EXPECT_EQ(iter.next(), "3");
    EXPECT_EQ(iter.next(), "");
    EXPECT_EQ(iter.next(), "4");
    EXPECT_EQ(iter.next(), "5-6");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, SplitAnyNonEmpty) {
    auto iter = splitAny("1,2:3;;4|5-6", ",:;|", true);

    EXPECT_EQ(iter.next(), "1");
    EXPECT_EQ(iter.next(), "2");
    EXPECT_EQ(iter.next(), "3");
    EXPECT_EQ(iter.next(), "4");
    EXPECT_EQ(iter.next(), "5-6");
    EXPECT_EQ(iter.next(), std::nullopt);
}


TEST(IterTests, SplitAnyEmptyStr) {
    auto iter = splitAny("", ",:;|");

    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, SplitAnyEmptyDelim) {
    auto iter = splitAny("hi", "");

    EXPECT_EQ(iter.next(), "hi");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, SplitWhitespace) {
    auto iter = splitWhitespace("hi   test\n hello\r\nyay");

    EXPECT_EQ(iter.next(), "hi");
    EXPECT_EQ(iter.next(), "test");
    EXPECT_EQ(iter.next(), "hello");
    EXPECT_EQ(iter.next(), "yay");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Empty) {
    auto iter = empty<int*>();
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Once) {
    auto iter = once(42);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, RepeatN) {
    auto iter = repeat(42, 3);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, RepeatForever) {
    auto iter = repeat(42);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), 42);
    EXPECT_EQ(iter.next(), 42);
    // and so on...
}

TEST(IterTests, Cycle) {
    std::vector<int> vec = {1, 2, 3};
    auto iter = from(vec).cycle();
    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), 1);
}

TEST(IterTests, ArrayChunks) {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto iter = from(vec).arrayChunks<3>();
    EXPECT_EQ(iter.next(), (std::array<int, 3>{1, 2, 3}));
    EXPECT_EQ(iter.next(), (std::array<int, 3>{4, 5, 6}));
    EXPECT_EQ(iter.next(), (std::array<int, 3>{7, 8, 9}));
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, CxxIterRangeLoop) {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    int sum = 0;

    for (int x : from(vec).map([](int x) { return x * 2; })) {
        sum += x;
    }

    EXPECT_EQ(sum, 30);
}

TEST(IterTests, CxxIterLoop) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec).map([](int x) { return x * 2; });

    int sum = 0;

    for (auto it = iter.begin(); it != iter.end(); ++it) {
        sum += *it;
    }

    EXPECT_EQ(sum, 30);
}

TEST(IterTests, CxxIterMut) {
    std::vector<std::string> vec = {STRING_1, STRING_2, STRING_3};
    auto iter = from(vec).copied().collect();

    EXPECT_EQ(iter.size(), 3);
    EXPECT_EQ(iter[0], STRING_1);
    EXPECT_EQ(iter[1], STRING_2);
    EXPECT_EQ(iter[2], STRING_3);

    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], STRING_1);
    EXPECT_EQ(vec[1], STRING_2);
    EXPECT_EQ(vec[2], STRING_3);
}

TEST(IterTests, CxxIterMut2) {
    std::vector<std::string> vec = {STRING_1, STRING_2, STRING_3};
    auto iter = from(vec).enumerate().collect();

    EXPECT_EQ(iter.size(), 3);
    EXPECT_EQ(iter[0].second, STRING_1);
    EXPECT_EQ(iter[1].second, STRING_2);
    EXPECT_EQ(iter[2].second, STRING_3);

    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], STRING_1);
    EXPECT_EQ(vec[1], STRING_2);
    EXPECT_EQ(vec[2], STRING_3);
}

TEST(IterTests, CxxIterConsume) {
    std::vector<std::string> vec = {STRING_1, STRING_2, STRING_3};
    auto vec2 = consume(vec).collect();

    EXPECT_EQ(vec2.size(), 3);
    EXPECT_EQ(vec2[0], STRING_1);
    EXPECT_EQ(vec2[1], STRING_2);
    EXPECT_EQ(vec2[2], STRING_3);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], "");
    EXPECT_EQ(vec[2], "");
}

TEST(IterTests, IterFind) {
    std::unordered_map<std::string, int> map{{"1", 10}, {"2", 20}, {"3", 30}};

    size_t val = from(map).find([](const auto& p) { return p.first == "2"; })->get().second;
    EXPECT_EQ(val, 20);
}

TEST(IterTests, EnumerateSteal) {
    constexpr auto long1 = "very long string to bypass sso1";
    constexpr auto long2 = "very long string to bypass sso2";
    constexpr auto long3 = "very long string to bypass sso3";

    std::vector<std::string> strings{ long1, long2, long3 };

    size_t iters = 0;
    for (auto [idx, str] : from(strings).enumerate()) {
        // this is a reference, no copies done here
        static_assert(std::is_same_v<decltype(str), std::string&>);

        EXPECT_EQ(str, "very long string to bypass sso" + std::to_string(idx + 1));
        iters++;
    }

    EXPECT_EQ(iters, 3);

    // check that the original vector didnt get mutated
    EXPECT_EQ(strings.size(), 3);

    EXPECT_EQ(strings[0], long1);
    EXPECT_EQ(strings[1], long2);
    EXPECT_EQ(strings[2], long3);
}

TEST(IterTests, EnumerateConsume) {
    constexpr auto long1 = "very long string to bypass sso1";
    constexpr auto long2 = "very long string to bypass sso2";
    constexpr auto long3 = "very long string to bypass sso3";

    std::vector<std::string> strings{ long1, long2, long3 };

    size_t iters = 0;
    for (auto [idx, str] : consume(strings).enumerate()) {
        static_assert(std::is_same_v<decltype(str), std::string>);

        EXPECT_EQ(str, "very long string to bypass sso" + std::to_string(idx + 1));
        iters++;
    }

    EXPECT_EQ(iters, 3);

    // check that the original vector got mutated
    EXPECT_EQ(strings.size(), 3);

    EXPECT_EQ(strings[0], "");
    EXPECT_EQ(strings[1], "");
    EXPECT_EQ(strings[2], "");
}

TEST(IterTests, LinesEmpty) {
    auto iter = lines("");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesOne) {
    auto iter = lines("line");
    EXPECT_EQ(iter.next(), "line");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesSingleLF) {
    auto iter = lines("\n");
    EXPECT_EQ(iter.next(), "");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesSingleCR) {
    auto iter = lines("\r");
    EXPECT_EQ(iter.next(), "\r");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesSingleCRLF) {
    auto iter = lines("\r\n");
    EXPECT_EQ(iter.next(), "");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesSingleWithLF) {
    auto iter = lines("line\n");
    EXPECT_EQ(iter.next(), "line");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesSingleWithCRLF) {
    auto iter = lines("line\r\n");
    EXPECT_EQ(iter.next(), "line");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesSingleWithCR) {
    auto iter = lines("line\r");
    EXPECT_EQ(iter.next(), "line\r");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesMultipleSplitByLF) {
    auto iter = lines("line1\nline2");
    EXPECT_EQ(iter.next(), "line1");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesMultipleSplitByLFWithTrailing) {
    auto iter = lines("line1\nline2\n");
    EXPECT_EQ(iter.next(), "line1");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesMultipleSplitByCRLF) {
    auto iter = lines("line1\r\nline2");
    EXPECT_EQ(iter.next(), "line1");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesMultipleSplitByCRLFWithTrailing) {
    auto iter = lines("line1\r\nline2\r\n");
    EXPECT_EQ(iter.next(), "line1");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesMultipleSplitByCRLFWithTrailingLF) {
    auto iter = lines("line1\r\nline2\n");
    EXPECT_EQ(iter.next(), "line1");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesMultipleSplitByTwoLF) {
    auto iter = lines("line1\n\nline2");
    EXPECT_EQ(iter.next(), "line1");
    EXPECT_EQ(iter.next(), "");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesMultipleWrappedInLF) {
    auto iter = lines("\nline1\nline2\n");
    EXPECT_EQ(iter.next(), "");
    EXPECT_EQ(iter.next(), "line1");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesConsecutiveCR) {
    auto iter = lines("line1\r\r\nline2");
    EXPECT_EQ(iter.next(), "line1\r");
    EXPECT_EQ(iter.next(), "line2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, LinesConsecutiveCRWithNoBreak) {
    auto iter = lines("line1\r\r\rline2");
    EXPECT_EQ(iter.next(), "line1\r\r\rline2");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Range) {
    auto r1 = range(0, 5);
    EXPECT_EQ(r1.next(), 0);
    EXPECT_EQ(r1.next(), 1);
    EXPECT_EQ(r1.next(), 2);
    EXPECT_EQ(r1.next(), 3);
    EXPECT_EQ(r1.next(), 4);
    EXPECT_EQ(r1.next(), std::nullopt);
}

TEST(IterTests, FlattenEmpty) {
    std::vector<std::vector<int>> vec = {};
    auto iter = from(vec).flatten();
    static_assert(std::is_same_v<decltype(iter)::Item, int>);

    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FlattenOneVector) {
    std::vector<std::vector<int>> vec = {{1, 2, 3}};
    auto iter = from(vec).flatten();

    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FlattenTwoVectors) {
    std::vector<std::vector<int>> vec = {{1, 2}, {3, 4}};
    auto iter = from(vec).flatten();

    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), 4);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FlattenMixedEmptyCollections) {
    std::vector<std::vector<int>> vec = {{}, {1, 2}, {}, {3, 4}, {}};
    auto iter = from(vec).flatten();

    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), 2);
    EXPECT_EQ(iter.next(), 3);
    EXPECT_EQ(iter.next(), 4);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FlattenIterMix) {
    // 3 ranges:
    // * 0..0 - empty and yields nothing
    // * 0..1 - yields 0
    // * 0..2 - yields 0, 1
    auto iter = range(0, 3)
        .map([](int x) { return range(0, x); })
        .flatten();

    EXPECT_EQ(iter.next(), 0);
    EXPECT_EQ(iter.next(), 0);
    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FlattenTextSplit) {
    // Split the following string by whitespace using solely flatMap,
    // even if this isn't super efficient, it is incredibly easy
    std::string text = "1\t2 3\n4 5\t6";

    auto iter = split(text, '\t')
        .flatMap([](auto sv) { return split(sv, ' '); })
        .flatMap([](auto sv) { return split(sv, '\n'); });

    EXPECT_EQ(iter.next(), "1");
    EXPECT_EQ(iter.next(), "2");
    EXPECT_EQ(iter.next(), "3");
    EXPECT_EQ(iter.next(), "4");
    EXPECT_EQ(iter.next(), "5");
    EXPECT_EQ(iter.next(), "6");
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FlatMapIterMix) {
    // 3 ranges:
    // * 0..0 - empty and yields nothing
    // * 0..1 - yields 0
    // * 0..2 - yields 0, 1
    auto iter = range(0, 3)
        .flatMap([](int x) { return range(0, x); });

    EXPECT_EQ(iter.next(), 0);
    EXPECT_EQ(iter.next(), 0);
    EXPECT_EQ(iter.next(), 1);
    EXPECT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FoldSum) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    int sum = from(vec).fold(0, [](int& acc, int x) { acc += x; });
    EXPECT_EQ(sum, 15);
}

TEST(IterTests, FoldSet) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto result = from(vec).fold(
        std::unordered_set<int>{},
        [](std::unordered_set<int>& acc, int x) { acc.insert(x); }
    );

    EXPECT_EQ(result.size(), 5);
    for (int i = 1; i <= 5; ++i) {
        EXPECT_TRUE(result.contains(i));
    }
}

TEST(IterTests, FoldStr) {
    std::vector<std::string> vec {"1", "22", "333"};
    auto result = from(vec).fold(
        size_t{0},
        [](size_t& acc, const std::string& x) { acc += x.size(); }
    );

    EXPECT_EQ(result, 6);
}

TEST(IterTests, Nth0) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::vector<int> vec2;
    std::vector<int> vec3 = {1};

    EXPECT_EQ(from(vec).nth(0), 1);
    EXPECT_EQ(from(vec2).nth(0), std::nullopt);
    EXPECT_EQ(from(vec3).nth(0), 1);
}

TEST(IterTests, Nth1) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::vector<int> vec2;
    std::vector<int> vec3 = {1};

    EXPECT_EQ(from(vec).nth(1), 2);
    EXPECT_EQ(from(vec2).nth(1), std::nullopt);
    EXPECT_EQ(from(vec3).nth(1), std::nullopt);
}

TEST(IterTests, NextChunk) {
    std::vector<int> vec1;
    std::vector<int> vec2 = {1};
    std::vector<int> vec3 = {1, 2};
    std::vector<int> vec4 = {1, 2, 3};
    std::vector<int> vec5 = {1, 2, 3, 4};

    auto it1 = from(vec1);
    EXPECT_EQ(it1.nextChunk<1>(), std::nullopt);
    EXPECT_EQ(it1.nextChunk<2>(), std::nullopt);

    auto it2 = from(vec2);
    EXPECT_EQ(it2.nextChunk<2>(), std::nullopt);

    auto it3 = from(vec3);
    EXPECT_EQ(it3.nextChunk<2>(), (std::array<int, 2>{1, 2}));
    EXPECT_EQ(it3.nextChunk<2>(), std::nullopt);

    auto it4 = from(vec4);
    EXPECT_EQ(it4.nextChunk<2>(), (std::array<int, 2>{1, 2}));
    EXPECT_EQ(it4.nextChunk<2>(), std::nullopt);

    auto it5 = from(vec5);
    EXPECT_EQ(it5.nextChunk<2>(), (std::array<int, 2>{1, 2}));
    EXPECT_EQ(it5.nextChunk<2>(), (std::array<int, 2>{3, 4}));
    EXPECT_EQ(it5.nextChunk<2>(), std::nullopt);
}

TEST(IterTests, NextChunkVarying) {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    auto it = from(vec);
    EXPECT_EQ(it.nextChunk<2>(), (std::array<int, 2>{1, 2}));
    EXPECT_EQ(it.nextChunk<3>(), (std::array<int, 3>{3, 4, 5}));
    EXPECT_EQ(it.nextChunk<1>(), std::nullopt);
}
