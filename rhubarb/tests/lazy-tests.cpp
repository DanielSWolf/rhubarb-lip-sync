#include <gmock/gmock.h>

#include "tools/lazy.h"

using namespace testing;

// Not copyable, no default constructor, movable
struct Foo {
    const int value;

    Foo(int value) :
        value(value) {}

    Foo() = delete;
    Foo(const Foo&) = delete;
    Foo& operator=(const Foo&) = delete;

    Foo(Foo&&) = default;
    Foo& operator=(Foo&&) = default;
};

TEST(Lazy, basicUsage) {
    bool lambdaCalled = false;
    Lazy<Foo> lazy([&lambdaCalled] {
        lambdaCalled = true;
        return Foo(42);
    });
    EXPECT_FALSE(lambdaCalled);
    EXPECT_FALSE(static_cast<bool>(lazy));
    EXPECT_EQ(42, (*lazy).value);
    EXPECT_EQ(42, lazy.value().value);
    EXPECT_EQ(42, lazy->value);
    EXPECT_TRUE(lambdaCalled);
    EXPECT_TRUE(static_cast<bool>(lazy));
}

TEST(Lazy, constUsage) {
    bool lambdaCalled = false;
    const Lazy<Foo> lazy([&lambdaCalled] {
        lambdaCalled = true;
        return Foo(42);
    });
    EXPECT_FALSE(lambdaCalled);
    EXPECT_FALSE(static_cast<bool>(lazy));
    EXPECT_EQ(42, (*lazy).value);
    EXPECT_EQ(42, lazy.value().value);
    EXPECT_EQ(42, lazy->value);
    EXPECT_TRUE(lambdaCalled);
    EXPECT_TRUE(static_cast<bool>(lazy));
}

TEST(Lazy, copying) {
    Lazy<Foo> a;
    int counter = 0;
    const auto createValue = [&] { return counter++; };
    Lazy<Foo> b(createValue);
    a = b;
    EXPECT_EQ(0, counter);
    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_FALSE(static_cast<bool>(b));
    EXPECT_EQ(0, a->value);
    EXPECT_EQ(1, counter);
    EXPECT_TRUE(static_cast<bool>(a));
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(0, b->value);
    Lazy<Foo> c(createValue);
    EXPECT_EQ(1, c->value);
}

using Expensive = Foo;
#define member value;

TEST(Lazy, demo) {
    // Constructor takes function
    Lazy<Expensive> lazy([] { return Expensive(42); });

    // Multiple ways to access value
    Expensive& a = *lazy;
    Expensive& b = lazy.value();
    auto c = lazy->member;

    // Check if initialized
    if (lazy) { /* ... */
    }
}
