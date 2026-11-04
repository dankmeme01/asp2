#include <asp/time.hpp>
#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <optional>
#include <string>
#include <string_view>

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
    auto dur = Duration::fromSecsF32(123.550f);

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
    auto dur7 = Duration::fromSecsF32(1.01f);
    auto dur8 = Duration::fromSecsF32(49.49f);
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

i64 utcEpochSeconds(
    i32 year, u32 month, u32 day,
    u32 hour = 0, u32 minute = 0, u32 second = 0
) {
    using namespace std::chrono;

    year_month_day date{
        std::chrono::year{year},
        std::chrono::month{month},
        std::chrono::day{day}
    };

    i64 days = sys_days{date}.time_since_epoch().count();
    return days * 86400
        + static_cast<i64>(hour) * 3600
        + static_cast<i64>(minute) * 60
        + static_cast<i64>(second);
}

TEST(SystemTimeTests, ParseEpoch) {
    auto parsed = SystemTime::parse("1970-01-01T00:00:00Z");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(*parsed, SystemTime::UNIX_EPOCH);
    EXPECT_EQ(parsed->timeSinceEpoch(), Duration::zero());
}

TEST(SystemTimeTests, ParseUtcMatchesChrono) {
    struct Case {
        const char* str;
        i32 year;
        u32 month, day, hour, minute, second;
    };

    static constexpr Case cases[] = {
        {"1970-01-01T00:00:00Z",  1970, 1,  1,  0,  0,  0},
        {"1970-01-01T00:00:01Z",  1970, 1,  1,  0,  0,  1},
        {"1970-12-31T23:59:59Z",  1970, 12, 31, 23, 59, 59},
        {"1971-01-01T00:00:00Z",  1971, 1,  1,  0,  0,  0},
        {"2000-02-29T12:00:00Z",  2000, 2,  29, 12, 0,  0},
        {"2000-03-01T00:00:00Z",  2000, 3,  1,  0,  0,  0},
        {"2024-01-01T00:00:00Z",  2024, 1,  1,  0,  0,  0},
        {"2024-02-29T23:59:59Z",  2024, 2,  29, 23, 59, 59},
        {"2024-12-31T23:59:59Z",  2024, 12, 31, 23, 59, 59},
        {"2100-03-01T00:00:00Z",  2100, 3,  1,  0,  0,  0},
        {"2400-02-29T00:00:00Z",  2400, 2,  29, 0,  0,  0},
    };

    for (const auto& c : cases) {
        auto parsed = SystemTime::parse(c.str);

        ASSERT_TRUE(parsed) << c.str;
        EXPECT_EQ(
            parsed->timeSinceEpoch().seconds(),
            static_cast<u64>(utcEpochSeconds(c.year, c.month, c.day, c.hour, c.minute, c.second))
        ) << c.str;
        EXPECT_EQ(parsed->timeSinceEpoch().subsecNanos(), u64{0}) << c.str;
    }
}

TEST(SystemTimeTests, ParseDateTimeUtc) {
    auto parsed = SystemTime::parse("2024-07-04T15:30:45Z");

    ASSERT_TRUE(parsed);

    auto dt = parsed->dateTimeUtc();
    EXPECT_EQ(dt.date.year, 2024u);
    EXPECT_EQ(static_cast<u32>(dt.date.month), 7u);
    EXPECT_EQ(static_cast<u32>(dt.date.day), 4u);
    EXPECT_EQ(static_cast<u32>(dt.time.hours), 15u);
    EXPECT_EQ(static_cast<u32>(dt.time.minutes), 30u);
    EXPECT_EQ(static_cast<u32>(dt.time.seconds), 45u);
}

TEST(SystemTimeTests, ParseTimezoneOffsets) {
    // Every string below denotes the same instant: 2024-06-15T12:34:56Z
    const i64 expected = utcEpochSeconds(2024, 6, 15, 12, 34, 56);

    static constexpr const char* cases[] = {
        "2024-06-15T12:34:56Z",
        "2024-06-15T12:34:56+00:00",
        "2024-06-15T12:34:56-00:00",
        "2024-06-15T18:04:56+05:30",
        "2024-06-15T04:34:56-08:00",
        "2024-06-15T14:34:56+02:00",
        "2024-06-15T12:04:56-00:30",
        "2024-06-15T18:04:56+0530",   // offset without a colon
        "2024-06-15T12:34:56+0000",
    };

    for (auto str : cases) {
        auto parsed = SystemTime::parse(str);

        ASSERT_TRUE(parsed) << str;
        EXPECT_EQ(parsed->timeSinceEpoch().seconds(), static_cast<u64>(expected)) << str;
    }
}

TEST(SystemTimeTests, ParseSubseconds) {
    struct Case {
        const char* str;
        u32 nanos;
    };

    static constexpr Case cases[] = {
        {"2024-01-01T00:00:00.0Z",         0},
        {"2024-01-01T00:00:00.00Z",        0},
        {"2024-01-01T00:00:00.1Z",         100'000'000},
        {"2024-01-01T00:00:00.12Z",        120'000'000},
        {"2024-01-01T00:00:00.123Z",       123'000'000},
        {"2024-01-01T00:00:00.123456Z",    123'456'000},
        {"2024-01-01T00:00:00.123456789Z", 123'456'789},
        {"2024-01-01T00:00:00.000000001Z", 1},
    };

    const i64 expectedSeconds = utcEpochSeconds(2024, 1, 1);

    for (const auto& c : cases) {
        auto parsed = SystemTime::parse(c.str);

        ASSERT_TRUE(parsed) << c.str;
        EXPECT_EQ(parsed->timeSinceEpoch().seconds(), static_cast<u64>(expectedSeconds)) << c.str;
        EXPECT_EQ(parsed->timeSinceEpoch().subsecNanos(), u64{c.nanos}) << c.str;
    }
}

TEST(SystemTimeTests, ParseAcceptedVariants) {
    const i64 expected = utcEpochSeconds(2024, 6, 15, 12, 34, 56);

    static constexpr const char* cases[] = {
        "2024-06-15T12:34:56Z",
        "2024-06-15 12:34:56Z",            // space instead of T
        "2024-06-15T12:34:56.000000000Z",
        "2024-06-15T12:34:56.0+00:00",
    };

    for (auto str : cases) {
        auto parsed = SystemTime::parse(str);

        ASSERT_TRUE(parsed) << str;
        EXPECT_EQ(parsed->timeSinceEpoch().seconds(), static_cast<u64>(expected)) << str;
    }
}

TEST(SystemTimeTests, ParseLeapSecond) {
    // :60 is accepted and normalized into the next minute
    auto parsed = SystemTime::parse("2024-01-01T23:59:60Z");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(
        parsed->timeSinceEpoch().seconds(),
        static_cast<u64>(utcEpochSeconds(2024, 1, 2, 0, 0, 0))
    );
}

TEST(SystemTimeTests, ParseRejectsInvalid) {
    static constexpr const char* cases[] = {
        "",
        "garbage",
        "2024-01-01",                       // date only
        "2024-01-01T00:00",                 // no seconds
        "20240101T000000Z",
        "2024-13-01T00:00:00Z",             // month out of range
        "2024-00-01T00:00:00Z",
        "2024-01-00T00:00:00Z",
        "2024-01-32T00:00:00Z",             // day out of range
        "2024-02-30T00:00:00Z",
        "2023-02-29T00:00:00Z",             // not a leap year
        "2100-02-29T00:00:00Z",             // not a leap year (century)
        "2024-04-31T00:00:00Z",
        "2024-01-01T24:00:00Z",             // hour out of range
        "2024-01-01T00:60:00Z",             // minute out of range
        "2024-01-01T00:00:61Z",             // second out of range
        "1969-12-31T23:59:59Z",             // before the epoch
        "1970-01-01T00:00:00+05:00",        // before the epoch after applying the offset
        "1970-01-01T00:00:00+00:01",
        "2024-01-01T00:00:00Zjunk",         // trailing characters
        "2024-01-01T00:00:00Z ",
        "2024-01-01T00:00:00+24:00",        // offset out of range
        "2024-01-01T00:00:00+99:00",
        "2024-01-01T00:00:00+00:60",
        "2024-01-01T00:00:00+5:00",         // offset not zero padded
        "2024-01-01T00:00:00,5Z",           // comma decimal separator
        "2024-01-01T00:00:00.Z",            // empty fraction
        "2024-01-01T00:00:00.1234567890Z",  // more than 9 fractional digits
        "2024-1-01T00:00:00Z",              // not zero padded
        "02024-01-01T00:00:00Z",            // 5 digit year
        "2024-01-01t00:00:00Z",             // lowercase t
        "2024-01-01T00:00:00z",             // lowercase z
    };

    for (auto str : cases) {
        EXPECT_FALSE(SystemTime::parse(str)) << str;
    }
}

TEST(SystemTimeTests, ParseRequireOffset) {
    auto parsedRequire = SystemTime::parse("1970-01-01T00:00:00", TzPolicy::Required);
    auto parsedOptional = SystemTime::parse("1970-01-01T00:00:00", TzPolicy::AssumeUtc);

    ASSERT_FALSE(parsedRequire);
    ASSERT_TRUE(parsedOptional);

    EXPECT_EQ(
        parsedOptional->timeSinceEpoch().seconds(),
        static_cast<u64>(utcEpochSeconds(1970, 1, 1, 0, 0, 0))
    );
}
