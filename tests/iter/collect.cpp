#include <asp/iter.hpp>
#include <gtest/gtest.h>
#include <unordered_set>

using namespace asp::iter;
using namespace asp;

static constexpr auto STRING_1 = "This is a long string 1 to bypass SSO";
static constexpr auto STRING_2 = "This is a long string 2 to bypass SSO";
static constexpr auto STRING_3 = "This is a long string 3 to bypass SSO";

TEST(IterTests, CollectVector) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto output = from(vec1)
        .map([](int x) { return x * 2; })
        .filter([](int x) { return x > 0; })
        .collect<std::vector<int>>();

    EXPECT_EQ(output, (std::vector<int>{2, 4, 6, 10}));
}

TEST(IterTests, CollectDeque) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto output = from(vec1)
        .collect<std::deque<int>>();

    EXPECT_EQ(output, (std::deque<int>{1, 2, 3, -4, 5}));
}

TEST(IterTests, CollectSet) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto output = from(vec1)
        .collect<std::set<int>>();

    EXPECT_EQ(output, (std::set<int>{-4, 1, 2, 3, 5}));
}

TEST(IterTests, CollectQueue) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto output = from(vec1)
        .collect<std::queue<int>>();

    EXPECT_EQ(output.size(), 5);
    EXPECT_EQ(output.front(), 1);
    EXPECT_EQ(output.back(), 5);
}

TEST(IterTests, CollectUSet) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto output = from(vec1)
        .collect<std::unordered_set<int>>();

    EXPECT_EQ(output.size(), 5);
    EXPECT_TRUE(output.contains(1));
    EXPECT_TRUE(output.contains(2));
    EXPECT_TRUE(output.contains(3));
    EXPECT_TRUE(output.contains(-4));
}

TEST(IterTests, CollectMap) {
    std::vector<std::string> vec1 = {"one", "two", "three"};

    auto output = from(vec1)
        .enumerate()
        .collect<std::map<size_t, std::string>>();

    EXPECT_EQ(output.size(), 3);
    EXPECT_EQ(output[0], "one");
    EXPECT_EQ(output[1], "two");
    EXPECT_EQ(output[2], "three");
}

TEST(IterTests, CollectUMap) {
    std::vector<std::string> vec1 = {"one", "two", "three"};

    auto output = from(vec1)
        .enumerate()
        .collect<std::unordered_map<size_t, std::string>>();

    EXPECT_EQ(output.size(), 3);
    EXPECT_EQ(output[0], "one");
    EXPECT_EQ(output[1], "two");
    EXPECT_EQ(output[2], "three");
}

TEST(IterTests, CollectDoesntSteal) {
    std::vector<std::string> vec = {STRING_1, STRING_2, STRING_3};

    auto output = from(vec).collect();

    // assert that we are collecting strings, not ref wrappers
    static_assert(std::is_same_v<decltype(vec), decltype(output)>);

    EXPECT_EQ(output.size(), 3);
    EXPECT_EQ(output[0], STRING_1);
    EXPECT_EQ(output[1], STRING_2);
    EXPECT_EQ(output[2], STRING_3);

    // assert the original vec is unchanged
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], STRING_1);
    EXPECT_EQ(vec[1], STRING_2);
    EXPECT_EQ(vec[2], STRING_3);
}

TEST(IterTests, CollectStealsWhenCan) {
    std::vector<std::string> vec = {STRING_1, STRING_2, STRING_3};

    auto output = consume(vec).collect();

    static_assert(std::is_same_v<decltype(vec), decltype(output)>);

    EXPECT_EQ(output.size(), 3);
    EXPECT_EQ(output[0], STRING_1);
    EXPECT_EQ(output[1], STRING_2);
    EXPECT_EQ(output[2], STRING_3);

    // assert the original items are moved away from
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], "");
    EXPECT_EQ(vec[2], "");
}

TEST(IterTests, CollectMapDoesntSteal) {
    std::vector<std::string> vec = {STRING_1, STRING_2, STRING_3};

    auto output = from(vec)
        .enumerate()
        .collect<std::map<size_t, std::string>>();

    static_assert(std::is_same_v<std::map<size_t, std::string>, decltype(output)>);

    EXPECT_EQ(output.size(), 3);
    EXPECT_EQ(output[0], STRING_1);
    EXPECT_EQ(output[1], STRING_2);
    EXPECT_EQ(output[2], STRING_3);

    // assert the original vec is unchanged
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], STRING_1);
    EXPECT_EQ(vec[1], STRING_2);
    EXPECT_EQ(vec[2], STRING_3);
}

TEST(IterTests, CollectMapStealsWhenCan) {
    std::vector<std::string> vec = {STRING_1, STRING_2, STRING_3};

    auto output = consume(vec)
        .enumerate()
        .collect<std::map<size_t, std::string>>();

    static_assert(std::is_same_v<std::map<size_t, std::string>, decltype(output)>);

    EXPECT_EQ(output.size(), 3);
    EXPECT_EQ(output[0], STRING_1);
    EXPECT_EQ(output[1], STRING_2);
    EXPECT_EQ(output[2], STRING_3);

    // assert the original items are moved away from
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], "");
    EXPECT_EQ(vec[2], "");
}

TEST(IterTests, CollectTakesSizeHint) {
    std::vector<std::string> items;

    // intentionally use NPOT size
    items.reserve(1025);
    for (size_t i = 0; i < 1025; i++) {
        items.push_back("test item");
    }

    // just to catch any funny errors, call a bunch of no-op transformations
    auto output = from(items)
        .map([](std::string& x) -> std::string& { return x; })
        .inspect([](std::string& x) { return; })
        .collect();

    // due to size hint being taken, the output vector should have same capacity instead of using geometric growth
    EXPECT_EQ(output.capacity(), 1025);
}

TEST(IterTests, CollectIgnoresSizeHintIfImpossible) {
    std::vector<std::string> items;

    // intentionally use NPOT size
    items.reserve(1025);
    for (size_t i = 0; i < 1025; i++) {
        items.push_back("test item");
    }

    // filter destroys the size hint, because at compile time it cannot be determined how many items will be transformed
    auto output = from(items)
        .filter([](std::string& x) { return true; })
        .collect();

    EXPECT_GT(output.capacity(), 1025);
}
