#include <asp/math.hpp>
#include <gtest/gtest.h>

using namespace asp;

TEST(NumberCycleTest, Basic) {
    NumberCycle cycle(0, 10);
    ++cycle;
    cycle++;

    EXPECT_EQ(cycle.get(), 2);
}

TEST(NumberCycleTest, BasicDec) {
    NumberCycle cycle(0, 10);
    ++cycle;
    cycle++;
    cycle--;
    cycle--;
    --cycle;

    EXPECT_EQ(cycle.get(), 10);
}

TEST(NumberCycleTest, Overflow) {
    NumberCycle cycle(0, 3);
    ++cycle;
    ++cycle;
    cycle++;
    ++cycle;

    EXPECT_EQ(cycle.get(), 0);
}

