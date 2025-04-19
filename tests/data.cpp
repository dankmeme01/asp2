#include <asp/data.hpp>
#include <gtest/gtest.h>

using namespace asp::data;
using namespace asp::nums;

TEST(CowTest, BorrowStringCtor) {
    CowString cow = CowString::fromBorrowed("hai this is a long string");
    EXPECT_TRUE(cow.isBorrowed());
    EXPECT_TRUE(!cow.isOwned());

    EXPECT_EQ(cow.asBorrowed(), "hai this is a long string");
    EXPECT_THROW(cow.asOwned(), std::bad_variant_access);
}

TEST(CowTest, OwnedStringCtor) {
    CowString cow = CowString::fromOwned("hai this is a long string");
    EXPECT_TRUE(!cow.isBorrowed());
    EXPECT_TRUE(cow.isOwned());

    EXPECT_EQ(cow.asOwned(), "hai this is a long string");
    EXPECT_THROW(cow.asBorrowed(), std::bad_variant_access);
}

TEST(CowTest, BorrowStringConverters) {
    CowString cow = CowString::fromBorrowed("hai this is a long string");

    decltype(auto) owned = cow.toOwned();
    static_assert(std::is_same_v<decltype(owned), std::string>, "type must be string after toOwned");
    EXPECT_EQ(owned, "hai this is a long string");

    EXPECT_TRUE(cow.isBorrowed()); // still borrowed

    decltype(auto) borrow2 = cow.toBorrowed();
    static_assert(std::is_same_v<decltype(borrow2), std::string_view>, "type must be string_view after toBorrowed");
    EXPECT_EQ(borrow2, "hai this is a long string");
}

TEST(CowTest, OwnedStringConverters) {
    CowString cow = CowString::fromOwned("hai this is a long string");

    decltype(auto) borrowed = cow.toBorrowed();
    static_assert(std::is_same_v<decltype(borrowed), std::string_view>, "type must be string_view after toBorrowed");
    EXPECT_EQ(borrowed, "hai this is a long string");

    EXPECT_TRUE(cow.isOwned()); // still owned

    decltype(auto) owned2 = cow.toOwned();
    EXPECT_EQ(owned2, "hai this is a long string");
    EXPECT_FALSE(cow.asOwned().data() == owned2.data()); // should be different instances
}

TEST(CowTest, BorrowStringConverterInPlace) {
    CowString cow = CowString::fromBorrowed("hai this is a long string");

    decltype(auto) cvted = cow.convertToOwned();
    decltype(auto) asowned = cow.asOwned();
    EXPECT_THROW(cow.asBorrowed(), std::bad_variant_access);

    EXPECT_EQ(cvted.data(), asowned.data());
    EXPECT_EQ(cvted, "hai this is a long string");
}

TEST(NumsTest, CheckedAdd) {
    u8 val1 = 200, val2 = 100, out;

    bool overflow = !asp::checkedAdd(out, val1, val2);
    EXPECT_TRUE(overflow);
    // contents of out are undefined at this point

    val1 = 100;
    val2 = 100;
    overflow = !asp::checkedAdd(out, val1, val2);
    EXPECT_FALSE(overflow);
    EXPECT_EQ(out, 200);
}

TEST(NumsTest, CheckedSub) {
    u8 val1 = 100, val2 = 200, out;

    bool overflow = !asp::checkedSub(out, val1, val2);
    EXPECT_TRUE(overflow);
    // contents of out are undefined at this point

    val1 = 200;
    val2 = 100;
    overflow = !asp::checkedSub(out, val1, val2);
    EXPECT_FALSE(overflow);
    EXPECT_EQ(out, 100);
}

TEST(NumsTest, CheckedMul) {
    u8 val1 = 100, val2 = 200, out;

    bool overflow = !asp::checkedMul(out, val1, val2);
    EXPECT_TRUE(overflow);
    // contents of out are undefined at this point

    val1 = 5;
    val2 = 10;
    overflow = !asp::checkedMul(out, val1, val2);
    EXPECT_FALSE(overflow);
    EXPECT_EQ(out, 50);
}

TEST(DataUtilTest, BitCast) {
    u32 value = 0x3f800000;
    float floatval = asp::data::bit_cast<f32>(value);

    EXPECT_EQ(floatval, 1.0f);
}

TEST(DataUtilTest, BitCast2) {
    u32 value = 0x1234;
    i32 signedval = asp::data::bit_cast<i32>(value);

    EXPECT_EQ(signedval, 0x1234);
}

TEST(DataUtilTest, ByteswapShort) {
    u16 value = 0x1234;
    u16 swapped = asp::data::byteswap(value);

    EXPECT_EQ(swapped, 0x3412);
}

TEST(DataUtilTest, ByteswapU32) {
    u32 value = 0x12345678;
    u32 swapped = asp::data::byteswap(value);

    EXPECT_EQ(swapped, 0x78563412);
}

TEST(DataUtilTest, ByteswapFloat) {
    f32 value = asp::data::bit_cast<f32, u32>(0x0000'803fU);
    f32 swapped = asp::data::byteswap(value);

    EXPECT_EQ(swapped, 1.0f);
}

TEST(DataUtilTest, ByteswapU64) {
    u64 value = 0x1234567890abcdef;
    u64 swapped = asp::data::byteswap(value);

    EXPECT_EQ(swapped, 0xefcd'ab90'7856'3412);
}

TEST(DataUtilTest, ByteswapDouble) {
    f64 value = asp::data::bit_cast<f64, u64>(0x0000'0000'0000'f03fULL);
    f64 swapped = asp::data::byteswap(value);

    EXPECT_EQ(swapped, 1.0);
}

TEST(DataUtilTest, NumParseU8) {
    EXPECT_EQ(asp::data::constexprParse<u8>("0"), 0);
    EXPECT_EQ(asp::data::constexprParse<u8>("12"), 12);
    EXPECT_EQ(asp::data::constexprParse<u8>("255"), 255);
    EXPECT_THROW(asp::data::constexprParse<u8>("1234"), std::runtime_error);
    EXPECT_THROW(asp::data::constexprParse<u8>("-11"), std::runtime_error);
    EXPECT_THROW(asp::data::constexprParse<u8>("-112345"), std::runtime_error);
}

TEST(DataUtilTest, NumParseI32) {
    EXPECT_EQ(asp::data::constexprParse<i32>("0"), 0);
    EXPECT_EQ(asp::data::constexprParse<i32>("54343653"), 54343653);
    EXPECT_EQ(asp::data::constexprParse<i32>("-12"), -12);
    EXPECT_EQ(asp::data::constexprParse<i32>("-2147483648"), -2147483648);
    EXPECT_EQ(asp::data::constexprParse<i32>("2147483647"), 2147483647);
    EXPECT_THROW(asp::data::constexprParse<i32>("-2147483649"), std::runtime_error);
    EXPECT_THROW(asp::data::constexprParse<i32>("2147483648"), std::runtime_error);
}

TEST(DataUtilTest, NumToStringU8) {
    EXPECT_EQ(asp::data::constexprToString<u8>(0), "0");
    EXPECT_EQ(asp::data::constexprToString<u8>(12), "12");
    EXPECT_EQ(asp::data::constexprToString<u8>(255), "255");
}

TEST(DataUtilTest, NumToStringI32) {
    EXPECT_EQ(asp::data::constexprToString<i32>(0), "0");
    EXPECT_EQ(asp::data::constexprToString<i32>(12), "12");
    EXPECT_EQ(asp::data::constexprToString<i32>(255), "255");
    EXPECT_EQ(asp::data::constexprToString<i32>(-1234), "-1234");
    EXPECT_EQ(asp::data::constexprToString<i32>(2147483647), "2147483647");
    EXPECT_EQ(asp::data::constexprToString<i32>(-2147483648), "-2147483648");
}
