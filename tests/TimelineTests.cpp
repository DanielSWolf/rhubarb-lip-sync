#include <gmock/gmock.h>
#include "Timeline.h"
#include <limits>
#include <functional>

using namespace testing;
using cs = centiseconds;
using std::vector;

TEST(Timeline, constructors_initializeState) {
	EXPECT_THAT(
		Timeline<int>(Timed<int>(cs(10), cs(30), 42)),
		ElementsAre(Timed<int>(cs(10), cs(30), 42))
	);
	EXPECT_THAT(
		Timeline<int>(TimeRange(cs(10), cs(30)), 42),
		ElementsAre(Timed<int>(cs(10), cs(30), 42))
	);
	EXPECT_THAT(
		Timeline<int>(cs(10), cs(30), 42),
		ElementsAre(Timed<int>(cs(10), cs(30), 42))
	);
	auto args = {
		Timed<int>(cs(-10), cs(30), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(50), cs(60), 3)
	};
	auto expected = {
		Timed<int>(cs(-10), cs(10), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(40), cs(50), 42),
		Timed<int>(cs(50), cs(60), 3)
	};
	EXPECT_THAT(
		Timeline<int>(args.begin(), args.end(), 42),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		Timeline<int>(args, 42),
		ElementsAreArray(expected)
	);
}

TEST(Timeline, constructors_throwForInvalidArgs) {
	EXPECT_THROW(
		Timeline<int>(cs(10), cs(9)),
		std::invalid_argument
	);
}

TEST(Timeline, empty) {
	Timeline<int> empty1{};
	EXPECT_TRUE(empty1.empty());
	EXPECT_THAT(empty1, IsEmpty());

	Timeline<int> empty2(cs(1), cs(1));
	EXPECT_TRUE(empty2.empty());
	EXPECT_THAT(empty2, IsEmpty());

	Timeline<int> nonEmpty(cs(1), cs(2));
	EXPECT_FALSE(nonEmpty.empty());
	EXPECT_THAT(nonEmpty, Not(IsEmpty()));
}

TEST(Timeline, size) {
	Timeline<int> empty1{};
	EXPECT_EQ(0, empty1.size());
	EXPECT_THAT(empty1, SizeIs(0));

	Timeline<int> empty2(cs(1), cs(1));
	EXPECT_EQ(0, empty2.size());
	EXPECT_THAT(empty2, SizeIs(0));

	Timeline<int> size1(cs(1), cs(10));
	EXPECT_EQ(1, size1.size());
	EXPECT_THAT(size1, SizeIs(1));

	Timeline<int> size2{Timed<int>(cs(-10), cs(10), 1), Timed<int>(cs(10), cs(11), 5)};
	EXPECT_EQ(2, size2.size());
	EXPECT_THAT(size2, SizeIs(2));
}

TEST(Timeline, getRange) {
	Timeline<int> empty1{};
	EXPECT_EQ(TimeRange(cs(0), cs(0)), empty1.getRange());

	Timeline<int> empty2(cs(1), cs(1));
	EXPECT_EQ(TimeRange(cs(1), cs(1)), empty2.getRange());

	Timeline<int> nonEmpty1(cs(1), cs(10));
	EXPECT_EQ(TimeRange(cs(1), cs(10)), nonEmpty1.getRange());

	Timeline<int> nonEmpty2{ Timed<int>(cs(-10), cs(10), 1), Timed<int>(cs(10), cs(11), 5) };
	EXPECT_EQ(TimeRange(cs(-10), cs(11)), nonEmpty2.getRange());
}

TEST(Timeline, iterators) {
	Timeline<int> timeline{ Timed<int>(cs(-5), cs(0), 10), Timed<int>(cs(5), cs(15), 9) };
	auto expected = { Timed<int>(cs(-5), cs(0), 10), Timed<int>(cs(0), cs(5), 0), Timed<int>(cs(5), cs(15), 9) };
	EXPECT_THAT(timeline, ElementsAreArray(expected));

	vector<Timed<int>> reversedActual;
	std::copy(timeline.rbegin(), timeline.rend(), back_inserter(reversedActual));
	vector<Timed<int>> reversedExpected;
	std::reverse_copy(expected.begin(), expected.end(), back_inserter(reversedExpected));
	EXPECT_THAT(reversedActual, ElementsAreArray(reversedExpected));
}

TEST(Timeline, find) {
	vector<Timed<int>> elements = {
		Timed<int>(cs(1), cs(2), 1),	// #0
		Timed<int>(cs(2), cs(4), 2),	// #1
		Timed<int>(cs(4), cs(5), 3)		// #2
	};
	Timeline<int> timeline(elements.begin(), elements.end());
	EXPECT_EQ(timeline.end(), timeline.find(cs(-1)));
	EXPECT_EQ(timeline.end(), timeline.find(cs(0)));
	EXPECT_EQ(elements[0], *timeline.find(cs(1)));
	EXPECT_EQ(elements[1], *timeline.find(cs(2)));
	EXPECT_EQ(elements[1], *timeline.find(cs(3)));
	EXPECT_EQ(elements[2], *timeline.find(cs(4)));
	EXPECT_EQ(timeline.end(), timeline.find(cs(5)));
}

TEST(Timeline, get) {
	vector<Timed<int>> elements = {
		Timed<int>(cs(1), cs(2), 1),	// #0
		Timed<int>(cs(2), cs(4), 2),	// #1
		Timed<int>(cs(4), cs(5), 3)		// #2
	};
	Timeline<int> timeline(elements.begin(), elements.end());
	EXPECT_THROW(timeline.get(cs(-1)), std::invalid_argument);
	EXPECT_THROW(timeline.get(cs(0)), std::invalid_argument);
	EXPECT_EQ(elements[0], timeline.get(cs(1)));
	EXPECT_EQ(elements[1], timeline.get(cs(2)));
	EXPECT_EQ(elements[1], timeline.get(cs(3)));
	EXPECT_EQ(elements[2], timeline.get(cs(4)));
	EXPECT_THROW(timeline.get(cs(5)), std::invalid_argument);
}

void testSetter(std::function<void(const Timed<int>&, Timeline<int>&)> set) {
	const Timed<int> initial(cs(0), cs(10), 42);
	Timeline<int> timeline(initial);
	vector<int> expectedValues(10, 42);
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
	for (const auto& newElement : newElements) {
		// Set element in timeline
		set(newElement, timeline);

		// Update expected value for every index
		cs elementStart = max(newElement.getStart(), cs(0));
		cs elementEnd = min(newElement.getEnd(), cs(10));
		for (cs t = elementStart; t < elementEnd; ++t) {
			expectedValues[t.count()] = newElement.getValue();
		}

		// Check timeline via indexer
		for (cs t = cs(0); t < cs(10); ++t) {
			EXPECT_EQ(expectedValues[t.count()], timeline[t]);
		}

		// Check timeline via iterators
		int lastValue = std::numeric_limits<int>::min();
		for (const auto& element : timeline) {
			// No element shound have zero-length
			EXPECT_LT(cs(0), element.getLength());

			// No two adjacent elements should have the same value; they should have been merged
			EXPECT_NE(lastValue, element.getValue());
			lastValue = element.getValue();

			// Element should match expected values
			for (cs t = element.getStart(); t < element.getEnd(); ++t) {
				EXPECT_EQ(expectedValues[t.count()], element.getValue());
			}
		}
	}
}

TEST(Timeline, set) {
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		timeline.set(element);
	});
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		timeline.set(element, element.getValue());
	});
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		timeline.set(element.getStart(), element.getEnd(), element.getValue());
	});
}

TEST(Timeline, indexer_set) {
	testSetter([](const Timed<int>& element, Timeline<int>& timeline) {
		for (cs t = element.getStart(); t < element.getEnd(); ++t) {
			if (t >= timeline.getRange().getStart() && t < timeline.getRange().getEnd()) {
				timeline[t] = element.getValue();
			} else {
				EXPECT_THROW(timeline[t] = element.getValue(), std::invalid_argument);
			}
		}
	});
}

TEST(Timeline, equality) {
	vector<Timeline<int>> timelines = {
		Timeline<int>{},
		Timeline<int>(cs(1), cs(1)),
		Timeline<int>(cs(1), cs(2)),
		Timeline<int>(cs(-10), cs(0))
	};

	for (size_t i = 0; i < timelines.size(); ++i) {
		for (size_t j = 0; j < timelines.size(); ++j) {
			if (i == j) {
				EXPECT_EQ(timelines[i], Timeline<int>(timelines[j]));
			} else {
				EXPECT_NE(timelines[i], timelines[j]);
			}
		}
	}
}