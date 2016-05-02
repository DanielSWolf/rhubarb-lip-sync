#include <gmock/gmock.h>
#include "Timeline.h"
#include <limits>
#include <functional>

using namespace testing;
using cs = centiseconds;
using std::vector;
using boost::optional;
using std::initializer_list;
using boost::none;

TEST(Timeline, constructors_initializeState) {
	auto args = {
		Timed<int>(cs(-10), cs(30), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(50), cs(60), 3)
	};
	auto expected = {
		Timed<int>(cs(-10), cs(10), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(50), cs(60), 3)
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

	Timeline<int> empty2{ Timed<int>(cs(1), cs(1), 1) };
	EXPECT_TRUE(empty2.empty());
	EXPECT_THAT(empty2, IsEmpty());

	Timeline<int> nonEmpty{ Timed<int>(cs(1), cs(2), 1) };
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

	Timeline<int> empty2{ Timed<int>(cs(1), cs(1), 1) };
	EXPECT_EQ(0, empty2.size());
	EXPECT_THAT(empty2, SizeIs(0));

	Timeline<int> size1{ Timed<int>(cs(1), cs(10), 1) };
	EXPECT_EQ(1, size1.size());
	EXPECT_THAT(size1, SizeIs(1));

	Timeline<int> size2{ Timed<int>(cs(-10), cs(10), 1), Timed<int>(cs(10), cs(11), 5) };
	EXPECT_EQ(2, size2.size());
	EXPECT_THAT(size2, SizeIs(2));
}

TEST(Timeline, getRange) {
	Timeline<int> empty0;
	EXPECT_EQ(TimeRange(cs(0), cs(0)), empty0.getRange());

	Timeline<int> empty1{};
	EXPECT_EQ(TimeRange(cs(0), cs(0)), empty1.getRange());

	Timeline<int> empty2{ Timed<int>(cs(1), cs(1), 1) };
	EXPECT_EQ(TimeRange(cs(0), cs(0)), empty2.getRange());

	Timeline<int> nonEmpty1{ Timed<int>(cs(1), cs(10), 1) };
	EXPECT_EQ(TimeRange(cs(1), cs(10)), nonEmpty1.getRange());

	Timeline<int> nonEmpty2{ Timed<int>(cs(-10), cs(5), 1), Timed<int>(cs(10), cs(11), 5) };
	EXPECT_EQ(TimeRange(cs(-10), cs(11)), nonEmpty2.getRange());
}

TEST(Timeline, iterators) {
	Timeline<int> timeline{ Timed<int>(cs(-5), cs(0), 10), Timed<int>(cs(5), cs(15), 9) };
	auto expected = { Timed<int>(cs(-5), cs(0), 10), Timed<int>(cs(5), cs(15), 9) };
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
		auto it = timeline.find(cs(++i), findMode);
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
	Timed<int> a = Timed<int>(cs(1), cs(2), 1);
	Timed<int> b = Timed<int>(cs(2), cs(5), 2);
	Timed<int> c = Timed<int>(cs(7), cs(9), 3);
	Timeline<int> timeline{ a, b, c };

	testFind(timeline, FindMode::SampleLeft, { nullptr, nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr });
	testFind(timeline, FindMode::SampleRight, { nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr, nullptr });
	testFind(timeline, FindMode::SearchLeft, { nullptr, nullptr, &a, &b, &b, &b, &b, &b, &c, &c, &c });
	testFind(timeline, FindMode::SearchRight, { &a, &a, &b, &b, &b, &c, &c, &c, &c, nullptr, nullptr });
}

TEST(Timeline, get) {
	Timed<int> a = Timed<int>(cs(1), cs(2), 1);
	Timed<int> b = Timed<int>(cs(2), cs(5), 2);
	Timed<int> c = Timed<int>(cs(7), cs(9), 3);
	Timeline<int> timeline{ a, b, c };

	initializer_list<Timed<int>*> expectedResults = { nullptr, &a, &b, &b, &b, nullptr, nullptr, &c, &c, nullptr, nullptr };
	int i = -1;
	for (Timed<int>* expectedResult : expectedResults) {
		optional<const Timed<int>&> value = timeline.get(cs(++i));
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
	Timeline<int> original{ { cs(1), cs(2), 1 }, { cs(2), cs(5), 2 }, { cs(7), cs(9), 3 } };
	
	{
		auto timeline = original;
		timeline.clear(cs(-10), cs(10));
		EXPECT_THAT(timeline, IsEmpty());
	}
	
	{
		auto timeline = original;
		timeline.clear(cs(1), cs(2));
		Timeline<int> expected{ { cs(2), cs(5), 2 }, { cs(7), cs(9), 3 } };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(cs(3), cs(4));
		Timeline<int> expected{ { cs(1), cs(2), 1 }, { cs(2), cs(3), 2 }, { cs(4), cs(5), 2}, { cs(7), cs(9), 3} };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(cs(6), cs(8));
		Timeline<int> expected{ { cs(1), cs(2), 1 }, { cs(2), cs(5), 2 }, { cs(8), cs(9), 3 } };
		EXPECT_EQ(expected, timeline);
	}
	
	{
		auto timeline = original;
		timeline.clear(cs(8), cs(10));
		Timeline<int> expected{ { cs(1), cs(2), 1 }, { cs(2), cs(5), 2 }, { cs(7), cs(8), 3 } };
		EXPECT_EQ(expected, timeline);
	}
}

void testSetter(std::function<void(const Timed<int>&, Timeline<int>&)> set) {
	Timeline<int> timeline;
	vector<optional<int>> expectedValues(20, none);
	auto newElements = {
		Timed<int>(cs(1), cs(2), 4),
		Timed<int>(cs(3), cs(6), 4),
		Timed<int>(cs(7), cs(9), 5),
		Timed<int>(cs(9), cs(10), 6),
		Timed<int>(cs(2), cs(3), 4),
		Timed<int>(cs(0), cs(1), 7),
		Timed<int>(cs(-10), cs(1), 8),
		Timed<int>(cs(-10), cs(0), 9),
		Timed<int>(cs(-10), cs(-1), 10),
		Timed<int>(cs(9), cs(20), 11),
		Timed<int>(cs(10), cs(20), 12),
		Timed<int>(cs(11), cs(20), 13),
		Timed<int>(cs(4), cs(6), 14),
		Timed<int>(cs(4), cs(6), 15),
		Timed<int>(cs(8), cs(10), 15),
		Timed<int>(cs(6), cs(8), 15),
		Timed<int>(cs(6), cs(8), 16)
	};
	int newElementIndex = -1;
	for (const auto& newElement : newElements) {
		++newElementIndex;
		// Set element in timeline
		set(newElement, timeline);

		// Update expected value for every index
		cs elementStart = max(newElement.getStart(), cs(0));
		cs elementEnd = newElement.getEnd();
		for (cs t = elementStart; t < elementEnd; ++t) {
			expectedValues[t.count()] = newElement.getValue();
		}

		// Check timeline via indexer
		for (cs t = cs(0); t < cs(10); ++t) {
			optional<const int&> actual = timeline[t];
			EXPECT_EQ(expectedValues[t.count()], actual ? optional<int>(*actual) : none);
		}

		// Check timeline via iterators
		Timed<int> lastElement(cs::min(), cs::min(), std::numeric_limits<int>::min());
		for (const auto& element : timeline) {
			// No element shound have zero-length
			EXPECT_LT(cs(0), element.getTimeRange().getLength());

			// No two adjacent elements should have the same value; they should have been merged
			if (element.getStart() == lastElement.getEnd()) {
				EXPECT_NE(lastElement.getValue(), element.getValue());
			}
			lastElement = element;

			// Element should match expected values
			for (cs t = std::max(cs::zero(), element.getStart()); t < element.getEnd(); ++t) {
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
	Timeline<int> timeline{ { cs(1), cs(2), 1 }, { cs(2), cs(4), 2 }, { cs(6), cs(9), 3 } };
	vector<optional<int>> expectedValues{ none, 1, 2, 2, none, none, 3, 3, 3 };
	for (cs t = cs(0); t < cs(9); ++t) {
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
		for (cs t = element.getStart(); t < element.getEnd(); ++t) {
			timeline[t] = element.getValue();
		}
	});
}

TEST(Timeline, shift) {
	Timeline<int> timeline{ { cs(1), cs(2), 1 },{ cs(2), cs(5), 2 },{ cs(7), cs(9), 3 } };
	Timeline<int> expected{ { cs(3), cs(4), 1 },{ cs(4), cs(7), 2 },{ cs(9), cs(11), 3 } };
	timeline.shift(cs(2));
	EXPECT_EQ(expected, timeline);
}

TEST(Timeline, equality) {
	vector<Timeline<int>> timelines = {
		Timeline<int>{},
		Timeline<int>{ { cs(1), cs(2), 0 } },
		Timeline<int>{ { cs(1), cs(2), 1 } },
		Timeline<int>{ { cs(-10), cs(0), 0 } }
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
