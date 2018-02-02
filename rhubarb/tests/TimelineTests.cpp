#include <gmock/gmock.h>
#include "time/Timeline.h"
#include <limits>
#include <functional>

using namespace testing;
using std::vector;
using boost::optional;
using std::initializer_list;
using boost::none;

TEST(Timeline, constructors_initializeState) {
	auto args = {
		Timed<int>(-10_cs, 30_cs, 1),
		Timed<int>(10_cs, 40_cs, 2),
		Timed<int>(50_cs, 60_cs, 3)
	};
	auto expected = {
		Timed<int>(-10_cs, 10_cs, 1),
		Timed<int>(10_cs, 40_cs, 2),
		Timed<int>(50_cs, 60_cs, 3)
	};
	EXPECT_THAT(
		Timeline<int>(args.begin(), args.end()),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		Timeline<int>(vector<Timed<int>>(args)),
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

	Timeline<int> empty2{ Timed<int>(1_cs, 1_cs, 1) };
	EXPECT_TRUE(empty2.empty());
	EXPECT_THAT(empty2, IsEmpty());

	Timeline<int> nonEmpty{ Timed<int>(1_cs, 2_cs, 1) };
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

	Timeline<int> empty2{ Timed<int>(1_cs, 1_cs, 1) };
	EXPECT_EQ(0, empty2.size());
	EXPECT_THAT(empty2, SizeIs(0));

	Timeline<int> size1{ Timed<int>(1_cs, 10_cs, 1) };
	EXPECT_EQ(1, size1.size());
	EXPECT_THAT(size1, SizeIs(1));

	Timeline<int> size2{ Timed<int>(-10_cs, 10_cs, 1), Timed<int>(10_cs, 11_cs, 5) };
	EXPECT_EQ(2, size2.size());
	EXPECT_THAT(size2, SizeIs(2));
}

TEST(Timeline, getRange) {
	Timeline<int> empty0;
	EXPECT_EQ(TimeRange(0_cs, 0_cs), empty0.getRange());

	Timeline<int> empty1{};
	EXPECT_EQ(TimeRange(0_cs, 0_cs), empty1.getRange());

	Timeline<int> empty2{ Timed<int>(1_cs, 1_cs, 1) };
	EXPECT_EQ(TimeRange(0_cs, 0_cs), empty2.getRange());

	Timeline<int> nonEmpty1{ Timed<int>(1_cs, 10_cs, 1) };
	EXPECT_EQ(TimeRange(1_cs, 10_cs), nonEmpty1.getRange());

	Timeline<int> nonEmpty2{ Timed<int>(-10_cs, 5_cs, 1), Timed<int>(10_cs, 11_cs, 5) };
	EXPECT_EQ(TimeRange(-10_cs, 11_cs), nonEmpty2.getRange());
}

TEST(Timeline, iterators) {
	Timeline<int> timeline{ Timed<int>(-5_cs, 0_cs, 10), Timed<int>(5_cs, 15_cs, 9) };
	auto expected = { Timed<int>(-5_cs, 0_cs, 10), Timed<int>(5_cs, 15_cs, 9) };
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
	Timed<int> a = Timed<int>(1_cs, 2_cs, 1);
	Timed<int> b = Timed<int>(2_cs, 5_cs, 2);
	Timed<int> c = Timed<int>(7_cs, 9_cs, 3);
	Timeline<int> timeline{ a, b, c };

	testFind(timeline, FindMode::SampleLeft, { nullptr, nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr });
	testFind(timeline, FindMode::SampleRight, { nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr, nullptr });
	testFind(timeline, FindMode::SearchLeft, { nullptr, nullptr, &a, &b, &b, &b, &b, &b, &c, &c, &c });
	testFind(timeline, FindMode::SearchRight, { &a, &a, &b, &b, &b, &c, &c, &c, &c, nullptr, nullptr });
}

TEST(Timeline, get) {
	Timed<int> a = Timed<int>(1_cs, 2_cs, 1);
	Timed<int> b = Timed<int>(2_cs, 5_cs, 2);
	Timed<int> c = Timed<int>(7_cs, 9_cs, 3);
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
	Timeline<int> original{ { 1_cs, 2_cs, 1 }, { 2_cs, 5_cs, 2 }, { 7_cs, 9_cs, 3 } };
	
	{
		auto timeline = original;
		timeline.clear(-10_cs, 10_cs);
		EXPECT_THAT(timeline, IsEmpty());
	}
	
	{
		auto timeline = original;
		timeline.clear(1_cs, 2_cs);
		Timeline<int> expected{ { 2_cs, 5_cs, 2 }, { 7_cs, 9_cs, 3 } };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(3_cs, 4_cs);
		Timeline<int> expected{ { 1_cs, 2_cs, 1 }, { 2_cs, 3_cs, 2 }, { 4_cs, 5_cs, 2}, { 7_cs, 9_cs, 3} };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(6_cs, 8_cs);
		Timeline<int> expected{ { 1_cs, 2_cs, 1 }, { 2_cs, 5_cs, 2 }, { 8_cs, 9_cs, 3 } };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(8_cs, 10_cs);
		Timeline<int> expected{ { 1_cs, 2_cs, 1 }, { 2_cs, 5_cs, 2 }, { 7_cs, 8_cs, 3 } };
		EXPECT_EQ(expected, timeline);
	}
}

void testSetter(std::function<void(const Timed<int>&, Timeline<int>&)> set) {
	Timeline<int> timeline;
	vector<optional<int>> expectedValues(20, none);
	auto newElements = {
		Timed<int>(1_cs, 2_cs, 4),
		Timed<int>(3_cs, 6_cs, 4),
		Timed<int>(7_cs, 9_cs, 5),
		Timed<int>(9_cs, 10_cs, 6),
		Timed<int>(2_cs, 3_cs, 4),
		Timed<int>(0_cs, 1_cs, 7),
		Timed<int>(-10_cs, 1_cs, 8),
		Timed<int>(-10_cs, 0_cs, 9),
		Timed<int>(-10_cs, -1_cs, 10),
		Timed<int>(9_cs, 20_cs, 11),
		Timed<int>(10_cs, 20_cs, 12),
		Timed<int>(11_cs, 20_cs, 13),
		Timed<int>(4_cs, 6_cs, 14),
		Timed<int>(4_cs, 6_cs, 15),
		Timed<int>(8_cs, 10_cs, 15),
		Timed<int>(6_cs, 8_cs, 15),
		Timed<int>(6_cs, 8_cs, 16)
	};
	int newElementIndex = -1;
	for (const auto& newElement : newElements) {
		++newElementIndex;
		// Set element in timeline
		set(newElement, timeline);

		// Update expected value for every index
		centiseconds elementStart = max(newElement.getStart(), 0_cs);
		centiseconds elementEnd = newElement.getEnd();
		for (centiseconds t = elementStart; t < elementEnd; ++t) {
			expectedValues[t.count()] = newElement.getValue();
		}

		// Check timeline via indexer
		for (centiseconds t = 0_cs; t < 10_cs; ++t) {
			optional<int> actual = timeline[t];
			EXPECT_EQ(expectedValues[t.count()], actual ? optional<int>(*actual) : none);
		}

		// Check timeline via iterators
		for (const auto& element : timeline) {
			// No element shound have zero-length
			EXPECT_LT(0_cs, element.getDuration());

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
	Timeline<int> timeline{ { 1_cs, 2_cs, 1 }, { 2_cs, 4_cs, 2 }, { 6_cs, 9_cs, 3 } };
	vector<optional<int>> expectedValues{ none, 1, 2, 2, none, none, 3, 3, 3 };
	for (centiseconds t = 0_cs; t < 9_cs; ++t) {
		{
			optional<int> actual = timeline[t];
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

TEST(Timeline, joinAdjacent) {
	Timeline<int> timeline{
		{1_cs, 2_cs, 1},
		{2_cs, 4_cs, 2},
		{3_cs, 6_cs, 2},
		{6_cs, 7_cs, 2},
		// Gap
		{8_cs, 10_cs, 2},
		{11_cs, 12_cs, 3}
	};
	EXPECT_EQ(6, timeline.size());
	timeline.joinAdjacent();
	EXPECT_EQ(4, timeline.size());
	
	Timed<int> expectedJoined[] = {
		{1_cs, 2_cs, 1},
		{2_cs, 7_cs, 2},
		// Gap
		{8_cs, 10_cs, 2},
		{11_cs, 12_cs, 3}
	};
	EXPECT_THAT(timeline, ElementsAreArray(expectedJoined));
}

TEST(Timeline, autoJoin) {
	JoiningTimeline<int> timeline{
		{1_cs, 2_cs, 1},
		{2_cs, 4_cs, 2},
		{3_cs, 6_cs, 2},
		{6_cs, 7_cs, 2},
		// Gap
		{8_cs, 10_cs, 2},
		{11_cs, 12_cs, 3}
	};
	Timed<int> expectedJoined[] = {
		{1_cs, 2_cs, 1},
		{2_cs, 7_cs, 2},
		// Gap
		{8_cs, 10_cs, 2},
		{11_cs, 12_cs, 3}
	};
	EXPECT_EQ(4, timeline.size());
	EXPECT_THAT(timeline, ElementsAreArray(expectedJoined));
}

TEST(Timeline, shift) {
	Timeline<int> timeline{ { 1_cs, 2_cs, 1 },{ 2_cs, 5_cs, 2 },{ 7_cs, 9_cs, 3 } };
	Timeline<int> expected{ { 3_cs, 4_cs, 1 },{ 4_cs, 7_cs, 2 },{ 9_cs, 11_cs, 3 } };
	timeline.shift(2_cs);
	EXPECT_EQ(expected, timeline);
}

TEST(Timeline, equality) {
	vector<Timeline<int>> timelines = {
		Timeline<int>{},
		Timeline<int>{ { 1_cs, 2_cs, 0 } },
		Timeline<int>{ { 1_cs, 2_cs, 1 } },
		Timeline<int>{ { -10_cs, 0_cs, 0 } }
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
