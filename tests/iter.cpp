#include <asp/iter.hpp>
#include <gtest/gtest.h>
#include <cmath>

using namespace asp::iter;
using namespace asp;

TEST(IterTests, VectorIter) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec);

    ASSERT_EQ(iter.next(), 1);
    ASSERT_EQ(iter.next(), 2);
    ASSERT_EQ(iter.next(), 3);
    ASSERT_EQ(iter.next(), 4);
    ASSERT_EQ(iter.next(), 5);
    ASSERT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Map) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec).map([](int x) { return x * 2; });

    ASSERT_EQ(iter.next(), 2);
    ASSERT_EQ(iter.next(), 4);
    ASSERT_EQ(iter.next(), 6);
    ASSERT_EQ(iter.next(), 8);
    ASSERT_EQ(iter.next(), 10);
    ASSERT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Filter) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec).filter([](int x) { return x % 2 == 0; });

    ASSERT_EQ(iter.next(), 2);
    ASSERT_EQ(iter.next(), 4);
    ASSERT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, FilterMap) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto iter = from(vec).filter([](int x) { return x % 2 == 0; }).map([](int x) { return x * 2; });

    ASSERT_EQ(iter.next(), 4);
    ASSERT_EQ(iter.next(), 8);
    ASSERT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Chain) {
    std::vector<int> vec1 = {1, 2, 3};
    std::vector<int> vec2 = {6, 7, 8};

    auto iter = from(vec1).chain(from(vec2));

    ASSERT_EQ(iter.next(), 1);
    ASSERT_EQ(iter.next(), 2);
    ASSERT_EQ(iter.next(), 3);
    ASSERT_EQ(iter.next(), 6);
    ASSERT_EQ(iter.next(), 7);
    ASSERT_EQ(iter.next(), 8);
    ASSERT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Take) {
    std::vector<int> vec1 = {1, 2, 3, 4, 5};

    auto iter = from(vec1).take(3);

    ASSERT_EQ(iter.next(), 1);
    ASSERT_EQ(iter.next(), 2);
    ASSERT_EQ(iter.next(), 3);
    ASSERT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, Zip) {
    std::vector<int> vec1 = {1, 2, 3};
    std::vector<double> vec2 = {1.0, 2.0, 3.0};

    auto iter = from(vec1).zip(from(vec2));

    ASSERT_EQ(iter.next(), std::make_pair(1, 1.0));
    ASSERT_EQ(iter.next(), std::make_pair(2, 2.0));
    ASSERT_EQ(iter.next(), std::make_pair(3, 3.0));
    ASSERT_EQ(iter.next(), std::nullopt);
}

TEST(IterTests, All) {
    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::vector<int> vec2 = {1, 2, 3, 4, 5, -4};

    ASSERT_TRUE(from(vec1).all([](int x) { return x > 0; }));
    ASSERT_FALSE(from(vec2).all([](int x) { return x > 0; }));
}

TEST(IterTests, Any) {
    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::vector<int> vec2 = {1, 2, 3, 4, 5, -4};

    ASSERT_FALSE(from(vec1).any([](int x) { return x < 0; }));
    ASSERT_TRUE(from(vec2).any([](int x) { return x < 0; }));
}

TEST(IterTests, Find) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};

    ASSERT_EQ(from(vec1).find([](int x) { return x < 0; }), -4);
    ASSERT_EQ(from(vec1).find([](int x) { return x > 5 ; }), std::nullopt);
}

TEST(IterTests, Collect) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto output = from(vec1)
        .map([](int x) { return x * 2; })
        .filter([](int x) { return x > 0; })
        .collect<std::vector<int>>();

    ASSERT_EQ(output, (std::vector<int>{2, 4, 6, 10}));
}

TEST(IterTests, Enumerate) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    auto iter = from(vec1).enumerate().collect<std::vector<std::pair<size_t, int>>>();

    ASSERT_EQ(iter, (std::vector<std::pair<size_t, int>>{{0, 1}, {1, 2}, {2, 3}, {3, -4}, {4, 5}}));
}

TEST(IterTests, ForEach) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    int sum = 0;

    from(vec1).forEach([&sum](int x) { sum += x; });
    ASSERT_EQ(sum, 7);
}

TEST(IterTests, Inspect) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    int sum = 0;

    from(vec1).inspect([&sum](int x) { sum += x; }).count();
    ASSERT_EQ(sum, 7);
}

TEST(IterTests, Min) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    ASSERT_EQ(from(vec1).min(), -4);
}

TEST(IterTests, Max) {
    std::vector<int> vec1 = {-1, 2, 333, -4, -49, 5, -100, 0};
    ASSERT_EQ(from(vec1).max(), 333);
}

TEST(IterTests, Sum) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    ASSERT_EQ(from(vec1).sum(), 7);
}

TEST(IterTests, HolyMix) {
    std::vector<int> vec1 = {1, 2, 3, -4, 5};
    std::vector<float> vec2 = {-1.5, -43, 3.14, 2.71, -0.42};
    std::vector<char> vec3 = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};

    std::string outstr;
    int outnum = 0;

    size_t count = from(vec1)
        .chain(from(vec2).map([](float x) { return static_cast<int>(round(x * x * x)); }))
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

    ASSERT_EQ(outnum, -79482);
    ASSERT_EQ(outstr, "egi");
}
