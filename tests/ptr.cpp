#include <asp/ptr.hpp>
#include <gtest/gtest.h>
#include <thread>

using namespace asp;

void drop(auto) {}

TEST(SharedPtrTest, BasicCreation) {
    auto ptr1 = make_shared<std::string>("hi test 47 sigmas");
    EXPECT_EQ(ptr1.strongCount(), 1);
    EXPECT_EQ(ptr1.weakCount(), 1);
    EXPECT_EQ(*ptr1, "hi test 47 sigmas");

    auto ptr2 = ptr1;
    EXPECT_EQ(ptr1.strongCount(), 2);
    EXPECT_EQ(ptr2.strongCount(), 2);
    EXPECT_EQ(ptr1, ptr2);

    drop(std::move(ptr1));
    EXPECT_EQ(ptr2.strongCount(), 1);
}

TEST(SharedPtrTest, Weak) {
    auto ptr1 = make_shared<std::string>("weak ptr test");
    WeakPtr<std::string> weak1 = ptr1;

    EXPECT_EQ(ptr1.strongCount(), 1);
    EXPECT_EQ(ptr1.weakCount(), 2);

    {
        auto upgraded = weak1.upgrade();
        EXPECT_EQ(upgraded.strongCount(), 2);
        EXPECT_EQ(*upgraded, "weak ptr test");
    }

    EXPECT_EQ(ptr1.strongCount(), 1);

    drop(std::move(ptr1));
    auto upgraded2 = weak1.upgrade();
    EXPECT_EQ(upgraded2.strongCount(), 0);
    EXPECT_EQ(upgraded2, nullptr);
}

TEST(SharedPtrTest, LeakTest) {
    static bool destroyed = false;

    struct LeakDetector {
        ~LeakDetector() { destroyed = true; }
    };

    WeakPtr<LeakDetector> weakPtr;
    {
        auto ptr = make_shared<LeakDetector>();
        weakPtr = ptr;
        EXPECT_FALSE(destroyed);
    }
    EXPECT_TRUE(destroyed);
}

TEST(SharedPtrTest, Polymorphism) {
    static bool baseD = false;
    static bool derD = false;

    struct Base {
        virtual ~Base() { baseD = true; }
    };
    struct Derived : public Base {
        ~Derived() override { derD = true; }
    };

    auto der = make_shared<Derived>();
    SharedPtr<Base> base = der;
    drop(std::move(der));

    EXPECT_FALSE(baseD);
    EXPECT_FALSE(derD);
    drop(std::move(base));

    EXPECT_TRUE(baseD);
    EXPECT_TRUE(derD);
}
