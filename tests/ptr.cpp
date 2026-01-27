#include <asp/ptr.hpp>
#include <gtest/gtest.h>

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

TEST(PtrSwapTest, Basic) {
    PtrSwap<std::string> swap;
    EXPECT_FALSE(swap.load());

    auto ptr1 = make_shared<std::string>("first");
    swap.store(ptr1);
    auto loaded1 = swap.load();
    EXPECT_EQ(*loaded1, "first");
    EXPECT_EQ(loaded1.strongCount(), 3); // ptr1 + loaded1 + swap

    auto ptr2 = make_shared<std::string>("second");
    swap.store(ptr2);
    auto loaded2 = swap.load();
    EXPECT_EQ(*loaded2, "second");
    EXPECT_EQ(ptr1.strongCount(), 2);
}

TEST(PtrSwapTest, Vec) {
    PtrSwap<std::vector<int>> swap;

    SharedPtr ptr1 = asp::make_shared<std::vector<int>>(std::vector<int>{1, 2, 3});
    swap.store(ptr1);

    SharedPtr ptr2 = swap.load();
    EXPECT_EQ(ptr2->size(), 3);
    EXPECT_EQ((*ptr2)[0], 1);
}