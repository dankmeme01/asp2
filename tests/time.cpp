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


TEST(DurationTests, Format) {
    auto dur1 = Duration::fromNanos(123);
    auto dur2 = Duration::fromNanos(12345);  // 12.345 us
    auto dur3 = Duration::fromMicros(497);
    auto dur4 = Duration::fromMillis(4);
    auto dur5 = Duration::fromMillis(42) + Duration::fromMicros(3);
    auto dur6 = Duration::fromMillis(420) + Duration::fromMicros(230);
    auto dur7 = Duration::fromSecsF32(1.01f).value();
    auto dur8 = Duration::fromSecsF32(49.49f).value();
    auto dur9 = Duration::fromSecs(42 * 60 + 29);
    auto dur10 = Duration::fromHours(2) + Duration::fromMinutes(15);

    EXPECT_EQ(dur1.toString(),  "123ns");
    EXPECT_EQ(dur2.toString(),  "12.345µs");
    EXPECT_EQ(dur3.toString(),  "497µs");
    EXPECT_EQ(dur4.toString(),  "4ms");
    EXPECT_EQ(dur5.toString(),  "42.003ms");
    EXPECT_EQ(dur6.toString(),  "420.23ms");
    EXPECT_EQ(dur7.toString(),  "1.01s");
    EXPECT_EQ(dur8.toString(),  "49.49s");
    EXPECT_EQ(dur9.toString(),  "42.483min");
    EXPECT_EQ(dur10.toString(), "2.25h");

    EXPECT_EQ(dur1.toHumanString(),  "123 nanoseconds");
    EXPECT_EQ(dur2.toHumanString(),  "12 microseconds");
    EXPECT_EQ(dur3.toHumanString(),  "497 microseconds");
    EXPECT_EQ(dur4.toHumanString(),  "4 milliseconds");
    EXPECT_EQ(dur5.toHumanString(),  "42 milliseconds");
    EXPECT_EQ(dur6.toHumanString(),  "420 milliseconds");
    EXPECT_EQ(dur7.toHumanString(),  "1 second");
    EXPECT_EQ(dur8.toHumanString(),  "49 seconds");
    EXPECT_EQ(dur9.toHumanString(),  "42 minutes");
    EXPECT_EQ(dur10.toHumanString(), "2 hours");

    // Check fmt
    EXPECT_EQ(fmt::to_string(dur1), dur1.toString());
    EXPECT_EQ(fmt::to_string(dur2), dur2.toString());
    EXPECT_EQ(fmt::to_string(dur3), dur3.toString());
    EXPECT_EQ(fmt::to_string(dur4), dur4.toString());
    EXPECT_EQ(fmt::to_string(dur5), dur5.toString());
    EXPECT_EQ(fmt::to_string(dur6), dur6.toString());
    EXPECT_EQ(fmt::to_string(dur7), dur7.toString());
    EXPECT_EQ(fmt::to_string(dur8), dur8.toString());
    EXPECT_EQ(fmt::to_string(dur9), dur9.toString());
    EXPECT_EQ(fmt::to_string(dur10), dur10.toString());
}

TEST(DurationTests, FormatPrecision) {
    auto dur6 = Duration::fromMillis(420) + Duration::fromMicros(230) + Duration::fromNanos(12);

    EXPECT_EQ(dur6.toString(0),  "420ms");
    EXPECT_EQ(dur6.toString(1),  "420.2ms");
    EXPECT_EQ(dur6.toString(2),  "420.23ms");
    EXPECT_EQ(dur6.toString(3),  "420.23ms");
    EXPECT_EQ(dur6.toString(4),  "420.23ms");
    EXPECT_EQ(dur6.toString(5),  "420.23001ms");
    EXPECT_EQ(dur6.toString(6),  "420.230012ms");
    EXPECT_EQ(dur6.toString(7),  "420.230012ms");
}

// TEST(SleepTests, SleepFor) {
//     auto dur1 = Duration::fromMillis(350);
//     auto instant = Instant::now();

//     asp::time::sleep(dur1);

//     auto taken = instant.elapsed();

//     auto diff = taken.absDiff(dur1);

//     EXPECT_LE(diff.millis(), 50);
// }

// TEST(SleepTests, SleepUntil) {
//     auto dur1 = Duration::fromMillis(350);
//     auto startTime = SystemTime::now();
//     auto estEndTime = startTime + dur1;

//     asp::time::sleepUntil(estEndTime);

//     auto endTime = SystemTime::now();

//     auto taken = (endTime - startTime).value();
//     auto diff = taken.absDiff(dur1);

//     EXPECT_LE(diff.millis(), 50);
// }

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

    EXPECT_NO_THROW({
        auto i2 = i1 + dur;
    });

    EXPECT_FALSE(i1.checkedAdd(dur).has_value());
}

TEST(InstantTests, SubOverflow) {
    auto i1 = Instant::now();
    auto dur = Duration::fromSecs(UINT64_MAX);

    EXPECT_NO_THROW({
        auto i2 = i1 - dur;
    });

    EXPECT_FALSE(i1.checkedSub(dur).has_value());
}
