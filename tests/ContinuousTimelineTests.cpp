#include <gmock/gmock.h>
#include "ContinuousTimeline.h"

using namespace testing;
using cs = centiseconds;
using std::vector;
using boost::optional;
using std::initializer_list;

TEST(ContinuousTimeline, constructors_initializeState) {
	TimeRange range(cs(-5), cs(55));
	int defaultValue = -1;
	auto args = {
		Timed<int>(cs(-10), cs(30), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(50), cs(60), 3)
	};
	auto expected = {
		Timed<int>(cs(-5), cs(10), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(40), cs(50), defaultValue),
		Timed<int>(cs(50), cs(55), 3)
	};
	EXPECT_THAT(
		ContinuousTimeline<int>(range, defaultValue, args.begin(), args.end()),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		ContinuousTimeline<int>(range, defaultValue, args),
		ElementsAreArray(expected)
	);
}

TEST(ContinuousTimeline, empty) {
	ContinuousTimeline<int> empty(TimeRange(cs(10), cs(10)), -1);
	EXPECT_TRUE(empty.empty());
	EXPECT_THAT(empty, IsEmpty());

	ContinuousTimeline<int> nonEmpty1(TimeRange(cs(0), cs(10)), -1);
	EXPECT_FALSE(nonEmpty1.empty());
	EXPECT_THAT(nonEmpty1, Not(IsEmpty()));

	ContinuousTimeline<int> nonEmpty2(TimeRange(cs(0), cs(10)), -1, { Timed<int>(cs(1), cs(2), 1) });
	EXPECT_FALSE(nonEmpty2.empty());
	EXPECT_THAT(nonEmpty2, Not(IsEmpty()));
}

TEST(ContinuousTimeline, setAndClear) {
	TimeRange range(cs(0), cs(10));
	int defaultValue = -1;
	ContinuousTimeline<int> timeline(range, defaultValue);

	// Out of range
	timeline.set(cs(-10), cs(-1), 1);
	timeline.set(TimeRange(cs(-5), cs(-1)), 2);
	timeline.set(Timed<int>(cs(10), cs(15), 3));

	// Overlapping
	timeline.set(cs(-2), cs(3), 4);
	timeline.set(TimeRange(cs(-1), cs(1)), 5);
	timeline.set(Timed<int>(cs(8), cs(12), 6));

	// Within
	timeline.set(cs(5), cs(9), 7);
	timeline.set(TimeRange(cs(6), cs(7)), 8);
	timeline.set(Timed<int>(cs(7), cs(8), 9));

	auto expected = {
		Timed<int>(cs(0), cs(1), 5),
		Timed<int>(cs(1), cs(3), 4),
		Timed<int>(cs(3), cs(5), defaultValue),
		Timed<int>(cs(5), cs(6), 7),
		Timed<int>(cs(6), cs(7), 8),
		Timed<int>(cs(7), cs(8), 9),
		Timed<int>(cs(8), cs(9), 7),
		Timed<int>(cs(9), cs(10), 6)
	};
	EXPECT_THAT(timeline, ElementsAreArray(expected));
}

TEST(ContinuousTimeline, shift) {
	ContinuousTimeline<int> timeline(TimeRange(cs(0), cs(10)), -1, { { cs(1), cs(2), 1 },{ cs(2), cs(5), 2 },{ cs(7), cs(9), 3 } });
	ContinuousTimeline<int> expected(TimeRange(cs(2), cs(12)), -1, { { cs(3), cs(4), 1 },{ cs(4), cs(7), 2 },{ cs(9), cs(11), 3 } });
	timeline.shift(cs(2));
	EXPECT_EQ(expected, timeline);
}

TEST(ContinuousTimeline, equality) {
	vector<ContinuousTimeline<int>> timelines = {
		ContinuousTimeline<int>(TimeRange(cs(0), cs(10)), -1),
		ContinuousTimeline<int>(TimeRange(cs(0), cs(10)), 1),
		ContinuousTimeline<int>(TimeRange(cs(0), cs(10)), -1, { { cs(1), cs(2), 1 } }),
		ContinuousTimeline<int>(TimeRange(cs(1), cs(10)), -1, { { cs(1), cs(2), 1 } })
	};

	for (size_t i = 0; i < timelines.size(); ++i) {
		for (size_t j = 0; j < timelines.size(); ++j) {
			if (i == j) {
				EXPECT_EQ(timelines[i], ContinuousTimeline<int>(timelines[j])) << "i: " << i << ", j: " << j;
			} else {
				EXPECT_NE(timelines[i], timelines[j]) << "i: " << i << ", j: " << j;
			}
		}
	}
}
