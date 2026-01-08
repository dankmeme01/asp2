#include <asp/collections.hpp>
#include <gtest/gtest.h>

using namespace asp;

TEST(SmallVecTest, Basic) {
    SmallVec<int, 4> vec;

    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 4);
    EXPECT_TRUE(vec.empty());

    vec.push_back(1);
    vec.push_back(2);

    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec.capacity(), 4);
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[0], vec.at(0));
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[1], vec.at(1));
    EXPECT_THROW(vec.at(2), std::out_of_range);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 1);

    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    vec.push_back(5);

    EXPECT_EQ(vec.size(), 5);
    EXPECT_GT(vec.capacity(), 4);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);
    EXPECT_THROW(vec.at(5), std::out_of_range);
}

TEST(SmallVecTest, MoveSemantics) {
    SmallVec<std::unique_ptr<int>, 2> vec;
    vec.push_back(std::make_unique<int>(10));
    vec.push_back(std::make_unique<int>(20));
    vec.push_back(std::make_unique<int>(30));

    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(*vec[0], 10);
    EXPECT_EQ(*vec[1], 20);
    EXPECT_EQ(*vec[2], 30);

    SmallVec<std::unique_ptr<int>, 2> vec2 = std::move(vec);
    EXPECT_EQ(vec2.size(), 3);
    EXPECT_EQ(*vec2[0], 10);
    EXPECT_EQ(*vec2[1], 20);
    EXPECT_EQ(*vec2[2], 30);

    EXPECT_EQ(vec.size(), 0);
}
