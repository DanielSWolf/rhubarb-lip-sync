#include <gmock/gmock.h>
#include "Timeline.h"
#include <limits>
#include <functional>

using namespace testing;
using std::vector;
using boost::optional;
using std::initializer_list;
using boost::none;

TEST(Timeline, constructors_initializeState) {
	auto args = {
		Timed<int>(-10cs, 30cs, 1),
		Timed<int>(10cs, 40cs, 2),
		Timed<int>(50cs, 60cs, 3)
	};
	auto expected = {
		Timed<int>(-10cs, 10cs, 1),
		Timed<int>(10cs, 40cs, 2),
		Timed<int>(50cs, 60cs, 3)
	};
	EXPECT_THAT(
		Timeline<int>(args.begin(), args.end()),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		Timeline<int>(args),
		ElementsAreArray(expected)
	);
}

TEST(Timeline, empty) {
	Timeline<int> empty0;
	EXPECT_TRUE(empty0.empty());
	EXPECT_THAT(empty0, IsEmpty());

	Timeline<int> empty1{};
	EXPECT_TRUE(empty1.empty());
	EXPECT_THAT(empty1, IsEmpty());

	Timeline<int> empty2{ Timed<int>(1cs, 1cs, 1) };
	EXPECT_TRUE(empty2.empty());
	EXPECT_THAT(empty2, IsEmpty());

	Timeline<int> nonEmpty{ Timed<int>(1cs, 2cs, 1) };
	EXPECT_FALSE(nonEmpty.empty());
	EXPECT_THAT(nonEmpty, Not(IsEmpty()));
}

TEST(Timeline, size) {
	Timeline<int> empty0;
	EXPECT_EQ(0, empty0.size());
	EXPECT_THAT(empty0, SizeIs(0));

	Timeline<int> empty1{};
	EXPECT_EQ(0, empty1.size());
	EXPECT_THAT(empty1, SizeIs(0));

	Timeline<int> empty2{ Timed<int>(1cs, 1cs, 1) };
	EXPECT_EQ(0, empty2.size());
	EXPECT_THAT(empty2, SizeIs(0));

	Timeline<int> size1{ Timed<int>(1cs, 10cs, 1) };
	EXPECT_EQ(1, size1.size());
	EXPECT_THAT(size1, SizeIs(1));

	Timeline<int> size2{ Timed<int>(-10cs, 10cs, 1), Timed<int>(10cs, 11cs, 5) };
	EXPECT_EQ(2, size2.size());
	EXPECT_THAT(size2, SizeIs(2));
}

TEST(Timeline, getRange) {
	Timeline<int> empty0;
	EXPECT_EQ(TimeRange(0cs, 0cs), empty0.getRange());

	Timeline<int> empty1{};
	EXPECT_EQ(TimeRange(0cs, 0cs), empty1.getRange());

	Timeline<int> empty2{ Timed<int>(1cs, 1cs, 1) };
	EXPECT_EQ(TimeRange(0cs, 0cs), empty2.getRange());

	Timeline<int> nonEmpty1{ Timed<int>(1cs, 10cs, 1) };
	EXPECT_EQ(TimeRange(1cs, 10cs), nonEmpty1.getRange());

	Timeline<int> nonEmpty2{ Timed<int>(-10cs, 5cs, 1), Timed<int>(10cs, 11cs, 5) };
	EXPECT_EQ(TimeRange(-10cs, 11cs), nonEmpty2.getRange());
}

TEST(Timeline, iterators) {
	Timeline<int> timeline{ Timed<int>(-5cs, 0cs, 10), Timed<int>(5cs, 15cs, 9) };
	auto expected = { Timed<int>(-5cs, 0cs, 10), Timed<int>(5cs, 15cs, 9) };
	EXPECT_THAT(timeline, ElementsAreArray(expected));

	vector<Timed<int>> reversedActual;
	copy(timeline.rbegin(), timeline.rend(), back_inserter(reversedActual));
	vector<Timed<int>> reversedExpected;
	reverse_copy(expected.begin(), expected.end(), back_inserter(reversedExpected));
	EXPECT_THAT(reversedActual, ElementsAreArray(reversedExpected));
}

void testFind(const Timeline<int>& timeline, FindMode findMode, const initializer_list<Timed<int>*> expectedResults) {
	int i = -1;
	for (Timed<int>* expectedResult : expectedResults) {
		auto it = timeline.find(centiseconds(++i), findMode);
		if (expectedResult != nullptr) {
			EXPECT_NE(it, timeline.end()) << "Timeline: " << timeline << "; findMode: " << static_cast<int>(findMode) << "; i: " << i;
			if (it != timeline.end()) {
				EXPECT_EQ(*expectedResult, *it) << "Timeline: " << timeline << "; findMode: " << static_cast<int>(findMode) << "; i: " << i;
			}
		} else {
			EXPECT_EQ(timeline.end(), it) << "Timeline: " << timeline << "; findMode: " << static_cast<int>(findMode) << "; i: " << i;
		}
	}
}

TEST(Timeline, find) {
	Timed<int> a = Timed<int>(1cs, 2cs, 1);
	Timed<int> b = Timed<int>(2cs, 5cs, 2);
	Timed<int> c = Timed<int>(7cs, 9cs, 3);
	Timeline<int> timeline{ a, b, c };

	testFind(timeline, FindMode::SampleLeft, { nullptr, nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr });
	testFind(timeline, FindMode::SampleRight, { nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr, nullptr });
	testFind(timeline, FindMode::SearchLeft, { nullptr, nullptr, &a, &b, &b, &b, &b, &b, &c, &c, &c });
	testFind(timeline, FindMode::SearchRight, { &a, &a, &b, &b, &b, &c, &c, &c, &c, nullptr, nullptr });
}

TEST(Timeline, get) {
	Timed<int> a = Timed<int>(1cs, 2cs, 1);
	Timed<int> b = Timed<int>(2cs, 5cs, 2);
	Timed<int> c = Timed<int>(7cs, 9cs, 3);
	Timeline<int> timeline{ a, b, c };

	initializer_list<Timed<int>*> expectedResults = { nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr, nullptr };
	int i = -1;
	for (Timed<int>* expectedResult : expectedResults) {
		optional<const Timed<int>&> value = timeline.get(centiseconds(++i));
		if (expectedResult != nullptr) {
			EXPECT_TRUE(value) << "i: " << i;
			if (value) {
				EXPECT_EQ(*expectedResult, *value) << "i: " << i;
			}
		} else {
			EXPECT_FALSE(value) << "i: " << i;
		}
	}
}

TEST(Timeline, clear) {
	Timeline<int> original{ { 1cs, 2cs, 1 }, { 2cs, 5cs, 2 }, { 7cs, 9cs, 3 } };
	
	{
		auto timeline = original;
		timeline.clear(-10cs, 10cs);
		EXPECT_THAT(timeline, IsEmpty());
	}
	
	{
		auto timeline = original;
		timeline.clear(1cs, 2cs);
		Timeline<int> expected{ { 2cs, 5cs, 2 }, { 7cs, 9cs, 3 } };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(3cs, 4cs);
		Timeline<int> expected{ { 1cs, 2cs, 1 }, { 2cs, 3cs, 2 }, { 4cs, 5cs, 2}, { 7cs, 9cs, 3} };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(6cs, 8cs);
		Timeline<int> expected{ { 1cs, 2cs, 1 }, { 2cs, 5cs, 2 }, { 8cs, 9cs, 3 } };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(8cs, 10cs);
		Timeline<int> expected{ { 1cs, 2cs, 1 }, { 2cs, 5cs, 2 }, { 7cs, 8cs, 3 } };
		EXPECT_EQ(expected, timeline);
	}
}

void testSetter(std::function<void(const Timed<int>&, Timeline<int>&)> set) {
	Timeline<int> timeline;
	vector<optional<int>> expectedValues(20, none);
	auto newElements = {
		Timed<int>(1cs, 2cs, 4),
		Timed<int>(3cs, 6cs, 4),
		Timed<int>(7cs, 9cs, 5),
		Timed<int>(9cs, 10cs, 6),
		Timed<int>(2cs, 3cs, 4),
		Timed<int>(0cs, 1cs, 7),
		Timed<int>(-10cs, 1cs, 8),
		Timed<int>(-10cs, 0cs, 9),
		Timed<int>(-10cs, -1cs, 10),
		Timed<int>(9cs, 20cs, 11),
		Timed<int>(10cs, 20cs, 12),
		Timed<int>(11cs, 20cs, 13),
		Timed<int>(4cs, 6cs, 14),
		Timed<int>(4cs, 6cs, 15),
		Timed<int>(8cs, 10cs, 15),
		Timed<int>(6cs, 8cs, 15),
		Timed<int>(6cs, 8cs, 16)
	};
	int newElementIndex = -1;
	for (const auto& newElement : newElements) {
		++newElementIndex;
		// Set element in timeline
		set(newElement, timeline);

		// Update expected value for every index
		centiseconds elementStart = max(newElement.getStart(), 0cs);
		centiseconds elementEnd = newElement.getEnd();
		for (centiseconds t = elementStart; t < elementEnd; ++t) {
			expectedValues[t.count()] = newElement.getValue();
		}

		// Check timeline via indexer
		for (centiseconds t = 0cs; t < 10cs; ++t) {
			optional<const int&> actual = timeline[t];
			EXPECT_EQ(expectedValues[t.count()], actual ? optional<int>(*actual) : none);
		}

		// Check timeline via iterators
		Timed<int> lastElement(centiseconds::min(), centiseconds::min(), std::numeric_limits<int>::min());
		for (const auto& element : timeline) {
			// No element shound have zero-length
			EXPECT_LT(0cs, element.getTimeRange().getLength());

			// No two adjacent elements should have the same value; they should have been merged
			if (element.getStart() == lastElement.getEnd()) {
				EXPECT_NE(lastElement.getValue(), element.getValue());
			}
			lastElement = element;

			// Element should match expected values
			for (centiseconds t = std::max(centiseconds::zero(), element.getStart()); t < element.getEnd(); ++t) {
				optional<int> expectedValue = expectedValues[t.count()];
				EXPECT_TRUE(expectedValue) << "Index " << t.count() << " should not have a value, but is within element " << element << ". "
					<< "newElementIndex: " << newElementIndex;
				if (expectedValue) {
					EXPECT_EQ(*expectedValue, element.getValue());
				}
			}
		}
	}
}

TEST(Timeline, set) {
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		timeline.set(element);
	});
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		timeline.set(element.getTimeRange(), element.getValue());
	});
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		timeline.set(element.getStart(), element.getEnd(), element.getValue());
	});
}

TEST(Timeline, indexer_get) {
	Timeline<int> timeline{ { 1cs, 2cs, 1 }, { 2cs, 4cs, 2 }, { 6cs, 9cs, 3 } };
	vector<optional<int>> expectedValues{ none, 1, 2, 2, none, none, 3, 3, 3 };
	for (centiseconds t = 0cs; t < 9cs; ++t) {
		{
			optional<const int&> actual = timeline[t];
			EXPECT_EQ(expectedValues[t.count()], actual ? optional<int>(*actual) : none);
		}
		{
			optional<int> actual = timeline[t];
			EXPECT_EQ(expectedValues[t.count()], actual ? optional<int>(*actual) : none);
		}
		if (expectedValues[t.count()]) {
			{
				const int& actual = timeline[t];
				EXPECT_EQ(*expectedValues[t.count()], actual);
			}
			{
				int actual = timeline[t];
				EXPECT_EQ(*expectedValues[t.count()], actual);
			}
		}
	}
}

TEST(Timeline, indexer_set) {
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		for (centiseconds t = element.getStart(); t < element.getEnd(); ++t) {
			timeline[t] = element.getValue();
		}
	});
}

TEST(Timeline, shift) {
	Timeline<int> timeline{ { 1cs, 2cs, 1 },{ 2cs, 5cs, 2 },{ 7cs, 9cs, 3 } };
	Timeline<int> expected{ { 3cs, 4cs, 1 },{ 4cs, 7cs, 2 },{ 9cs, 11cs, 3 } };
	timeline.shift(2cs);
	EXPECT_EQ(expected, timeline);
}

TEST(Timeline, equality) {
	vector<Timeline<int>> timelines = {
		Timeline<int>{},
		Timeline<int>{ { 1cs, 2cs, 0 } },
		Timeline<int>{ { 1cs, 2cs, 1 } },
		Timeline<int>{ { -10cs, 0cs, 0 } }
	};

	for (size_t i = 0; i < timelines.size(); ++i) {
		for (size_t j = 0; j < timelines.size(); ++j) {
			if (i == j) {
				EXPECT_EQ(timelines[i], Timeline<int>(timelines[j])) << "i: " << i << ", j: " << j;
			} else {
				EXPECT_NE(timelines[i], timelines[j]) << "i: " << i << ", j: " << j;
			}
		}
	}
}
