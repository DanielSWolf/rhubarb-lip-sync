#include <gmock/gmock.h>
#include "Lazy.h"

using namespace testing;
using std::make_unique;

// Not copyable, no default constrctor, movable
struct Foo {
	const int value;
	Foo(int value) : value(value) {}

	Foo() = delete;
	Foo(const Foo&) = delete;
	Foo& operator=(const Foo &) = delete;

	Foo(Foo&&) = default;
	Foo& operator=(Foo&&) = default;
};

TEST(Lazy, basicUsage) {
	bool lambdaCalled = false;
	Lazy<Foo> lazy([&lambdaCalled] { lambdaCalled = true; return Foo(42); });
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
	const Lazy<Foo> lazy([&lambdaCalled] { lambdaCalled = true; return Foo(42); });
	EXPECT_FALSE(lambdaCalled);
	EXPECT_FALSE(static_cast<bool>(lazy));
	EXPECT_EQ(42, (*lazy).value);
	EXPECT_EQ(42, lazy.value().value);
	EXPECT_EQ(42, lazy->value);
	EXPECT_TRUE(lambdaCalled);
	EXPECT_TRUE(static_cast<bool>(lazy));
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
	if (lazy) { /* ... */ }
}