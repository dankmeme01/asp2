#include <asp/time.hpp>
#include <gtest/gtest.h>

using namespace asp;

TEST(DurationTests, Ctor) {
    auto dur = Duration::fromSecs(123);

    EXPECT_EQ(dur.millis(), 123'000);
    EXPECT_EQ(dur.subsecMillis(), 0);
    EXPECT_EQ(dur.seconds(), 123);
    EXPECT_EQ(dur.minutes(), 2);
    EXPECT_EQ(dur.hours(), 0);
}

TEST(DurationTests, Ctor2) {
    auto dur = Duration::fromSecsF32(123.550f).value();

    EXPECT_EQ(dur.millis(), 123'550);
    EXPECT_EQ(dur.subsecMillis(), 550);
    EXPECT_EQ(dur.seconds(), 123);
    EXPECT_EQ(dur.minutes(), 2);
    EXPECT_EQ(dur.hours(), 0);
}

TEST(DurationTests, Ctor3) {
    // 1 billion micros = 1 million ms = 1550 seconds
    auto dur = Duration::fromMicros(1'550'678'123);

    EXPECT_EQ(dur.subsecMicros(), 678'123);
    EXPECT_EQ(dur.millis(), 1'550'678);
    EXPECT_EQ(dur.subsecMillis(), 678);
    EXPECT_EQ(dur.seconds(), 1550);
    EXPECT_EQ(dur.minutes(), 25);
    EXPECT_EQ(dur.hours(), 0);
}

TEST(DurationTests, AbsDiff) {
    auto dur1 = Duration::fromSecs(1234);
    auto dur2 = Duration::fromMillis(1274000);

    EXPECT_EQ(dur1.absDiff(dur2), Duration::fromSecs(40));
    EXPECT_EQ(dur2.absDiff(dur1), Duration::fromSecs(40));
}

TEST(DurationTests, Add) {
    auto dur1 = Duration::fromSecs(1234);
    auto dur2 = Duration::fromMillis(1274000);
    auto dur3 = dur1 + dur2;

    EXPECT_EQ(dur3.millis(), 2508000);

    dur3 += dur3;

    EXPECT_EQ(dur3.millis(), 5016000);
}

TEST(DurationTests, Sub) {
    auto dur1 = Duration::fromSecs(1234);
    auto dur2 = Duration::fromMillis(1274000);
    auto dur3 = dur2 - dur1;

    EXPECT_EQ(dur3.millis(), 40000);

    dur3 -= Duration::fromMillis(25000);

    EXPECT_EQ(dur3.millis(), 15000);
}

TEST(SleepTests, SleepFor) {
    auto dur1 = Duration::fromMillis(350);
    auto instant = Instant::now();

    asp::time::sleep(dur1);

    auto taken = instant.elapsed();

    auto diff = taken.absDiff(dur1);

    EXPECT_LE(diff.millis(), 50);
}

TEST(SleepTests, SleepUntil) {
    auto dur1 = Duration::fromMillis(350);
    auto startTime = SystemTime::now();
    auto estEndTime = startTime + dur1;

    asp::time::sleepUntil(estEndTime);

    auto endTime = SystemTime::now();

    auto taken = (endTime - startTime).value();
    auto diff = taken.absDiff(dur1);

    EXPECT_LE(diff.millis(), 50);
}

TEST(InstantTests, Cmp) {
    auto i1 = Instant::now();
    auto i2 = Instant::now();

    EXPECT_GE(i2, i1);
    EXPECT_LE(i1, i2);

    auto ad = i1.absDiff(i2);
    EXPECT_EQ(ad, i2.absDiff(i1));
    EXPECT_EQ(ad, i2.durationSince(i1));
}

TEST(InstantTests, AddSub) {
    auto i1 = Instant::now();
    auto dur = Duration::fromMillis(500);

    auto i2 = i1 + dur;
    auto i3 = i2 - dur;

    EXPECT_EQ(i1, i3);
    EXPECT_EQ(i2.durationSince(i1), dur);
    EXPECT_EQ(i1.durationSince(i2), Duration::zero());
}

TEST(InstantTests, AddOverflow) {
    auto i1 = Instant::farFuture();
    auto dur = Duration::fromSecs(UINT64_MAX);

    EXPECT_THROW({
        auto i2 = i1 + dur;
    }, std::runtime_error);

    EXPECT_FALSE(i1.checkedAdd(dur).has_value());
}

TEST(InstantTests, SubOverflow) {
    auto i1 = Instant::now();
    auto dur = Duration::fromSecs(UINT64_MAX);

    EXPECT_THROW({
        auto i2 = i1 - dur;
    }, std::runtime_error);

    EXPECT_FALSE(i1.checkedSub(dur).has_value());
}
