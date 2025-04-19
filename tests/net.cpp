#include <asp/net.hpp>
#include <gtest/gtest.h>

using namespace asp::net;
using namespace asp;

TEST(IpTest, v4Ctor) {
    Ipv4Address addr(1, 2, 3, 4);
    auto octets = std::array<u8, 4>{{1, 2, 3, 4}};

    EXPECT_EQ(addr.octets(), octets);
    EXPECT_EQ(addr.toBits(), 0x01020304);
    EXPECT_EQ(addr.toString(), "1.2.3.4");

    EXPECT_TRUE(!addr.isPrivate());
    EXPECT_TRUE(!addr.isBroadcast());
    EXPECT_TRUE(!addr.isLoopback());
    EXPECT_TRUE(!addr.isUnspecified());
}

TEST(IpTest, v4Equality) {
    Ipv4Address addr(1, 2, 3, 4);
    Ipv4Address addr2(1, 2, 3, 4);
    Ipv4Address addr3(5, 6, 7, 8);

    EXPECT_TRUE(addr == addr2);
    EXPECT_TRUE(addr != addr3);
    EXPECT_TRUE(addr2 != addr3);
}

TEST(IpTest, v4Constants) {
    EXPECT_EQ(Ipv4Address::LOCALHOST, Ipv4Address(127, 0, 0, 1));
    EXPECT_EQ(Ipv4Address::UNSPECIFIED, Ipv4Address{});
}

TEST(IpTest, v4Parse) {
    EXPECT_EQ(Ipv4Address::fromString("0.0.0.0"), Ipv4Address::UNSPECIFIED);
    EXPECT_EQ(Ipv4Address::fromString("127.0.0.1"), Ipv4Address::LOCALHOST);
    EXPECT_EQ(Ipv4Address::fromString("255.255.255.255"), Ipv4Address::BROADCAST);
    EXPECT_EQ(Ipv4Address::fromString("123.123.123.123"), Ipv4Address(123, 123, 123, 123));
    EXPECT_EQ(Ipv4Address::fromString("1.1.1.1"), Ipv4Address(1, 1, 1, 1));

    EXPECT_THROW(Ipv4Address::fromString("1"), std::runtime_error);
    EXPECT_THROW(Ipv4Address::fromString("1.2"), std::runtime_error);
    EXPECT_THROW(Ipv4Address::fromString("1.2.3"), std::runtime_error);
    EXPECT_THROW(Ipv4Address::fromString("1.2.3.256"), std::runtime_error);
    EXPECT_THROW(Ipv4Address::fromString("-1.0.0.0"), std::runtime_error);
    EXPECT_THROW(Ipv4Address::fromString("12.12.12.12:43"), std::runtime_error);
    EXPECT_THROW(Ipv4Address::fromString("256.256.256.256"), std::runtime_error);
    EXPECT_THROW(Ipv4Address::fromString("-1.-1.-1.-1"), std::runtime_error);
}

TEST(IpTest, GeneralTest) {
    IpAddress addr = IpAddress(Ipv4Address::LOCALHOST);

    EXPECT_TRUE(addr.isV4());
    EXPECT_EQ(addr.asV4(), Ipv4Address::LOCALHOST);
    EXPECT_THROW(addr.asV6(), std::runtime_error);
}

// IpRange

TEST(IpRangeTest, Ctor) {
    IpRange range(12, 12, 12, 12, 16);

    std::array<u8, 4> octets({12, 12, 12, 12});
    EXPECT_EQ(range.octets(), octets);
    EXPECT_EQ(range.prefixLength(), 16);
}

TEST(IpRangeTest, Parse) {
    IpRange range;

    range = IpRange::fromString("0.0.0.0/0");
    EXPECT_EQ(range.asIpv4(), Ipv4Address::fromString("0.0.0.0"));
    EXPECT_EQ(range.prefixLength(), 0);

    range = IpRange::fromString("127.127.127.127/12");
    EXPECT_EQ(range.asIpv4(), Ipv4Address::fromString("127.127.127.127"));
    EXPECT_EQ(range.prefixLength(), 12);

    range = IpRange::fromString("1.12.123.4/32");
    EXPECT_EQ(range.asIpv4(), Ipv4Address::fromString("1.12.123.4"));
    EXPECT_EQ(range.prefixLength(), 32);

    EXPECT_THROW(IpRange::fromString("-1.1.1.1/12"), std::runtime_error);
    EXPECT_THROW(IpRange::fromString("256.0.0.0/16"), std::runtime_error);
    EXPECT_THROW(IpRange::fromString("1.1.1.1/-1"), std::runtime_error);
    EXPECT_THROW(IpRange::fromString("8.8.8.8/33"), std::runtime_error);
    EXPECT_THROW(IpRange::fromString("1.1.1.1"), std::runtime_error);
    EXPECT_THROW(IpRange::fromString("0.0.0.0/"), std::runtime_error);
}

TEST(IpRangeTest, AsIpv4Mask) {
    IpRange range;

    range = IpRange::fromString("0.0.0.0/0");
    EXPECT_EQ(range.asIpv4Mask(), Ipv4Address::fromString("0.0.0.0"));

    range = IpRange::fromString("255.128.128.128/0");
    EXPECT_EQ(range.asIpv4Mask(), Ipv4Address::fromString("0.0.0.0"));

    range = IpRange::fromString("127.127.127.127/12");
    EXPECT_EQ(range.asIpv4Mask(), Ipv4Address::fromString("127.112.0.0"));

    range = IpRange::fromString("1.12.123.4/32");
    EXPECT_EQ(range.asIpv4Mask(), Ipv4Address::fromString("1.12.123.4"));

    range = IpRange::fromString("1.12.123.4/16");
    EXPECT_EQ(range.asIpv4Mask(), Ipv4Address::fromString("1.12.0.0"));

    range = IpRange::fromString("192.168.56.120/28");
    EXPECT_EQ(range.asIpv4Mask(), Ipv4Address::fromString("192.168.56.112"));
}

TEST(IpRangeTest, Contains) {
    IpRange range;

    range = IpRange::fromString("0.0.0.0/0");
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("127.0.0.1")));
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("0.0.0.1")));
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("255.255.255.255")));

    range = IpRange::fromString("255.128.128.128/8");
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("255.0.0.0")));
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("255.255.255.255")));
    EXPECT_FALSE(range.contains(Ipv4Address::fromString("254.255.255.255")));
    EXPECT_FALSE(range.contains(Ipv4Address::fromString("254.0.0.0")));
    EXPECT_FALSE(range.contains(Ipv4Address::fromString("1.0.0.0")));
    EXPECT_FALSE(range.contains(Ipv4Address::fromString("0.0.0.1")));

    range = IpRange::fromString("192.168.56.112/28");
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("192.168.56.112")));
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("192.168.56.118")));
    EXPECT_TRUE(range.contains(Ipv4Address::fromString("192.168.56.127")));
    EXPECT_FALSE(range.contains(Ipv4Address::fromString("192.168.56.128")));
    EXPECT_FALSE(range.contains(Ipv4Address::fromString("192.168.57.112")));
}

TEST(SocketAddressTest, Parse) {
    auto addr = SocketAddressV4::fromString("127.0.0.1:5421");
    EXPECT_EQ(addr.address().toString(), "127.0.0.1");
    EXPECT_EQ(addr.port(), 5421);
}
