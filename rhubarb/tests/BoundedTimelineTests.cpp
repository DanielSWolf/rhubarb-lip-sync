#include <gmock/gmock.h>
#include "time/BoundedTimeline.h"

using namespace testing;
using std::vector;
using boost::optional;
using std::initializer_list;

TEST(BoundedTimeline, constructors_initializeState) {
	TimeRange range(-5_cs, 55_cs);
	auto args = {
		Timed<int>(-10_cs, 30_cs, 1),
		Timed<int>(10_cs, 40_cs, 2),
		Timed<int>(50_cs, 60_cs, 3)
	};
	auto expected = {
		Timed<int>(-5_cs, 10_cs, 1),
		Timed<int>(10_cs, 40_cs, 2),
		Timed<int>(50_cs, 55_cs, 3)
	};
	EXPECT_THAT(
		BoundedTimeline<int>(range, args.begin(), args.end()),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		BoundedTimeline<int>(range, vector<Timed<int>>(args)),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		BoundedTimeline<int>(range, args),
		ElementsAreArray(expected)
	);
}

TEST(BoundedTimeline, empty) {
	BoundedTimeline<int> empty(TimeRange(0_cs, 10_cs));
	EXPECT_TRUE(empty.empty());
	EXPECT_THAT(empty, IsEmpty());

	BoundedTimeline<int> nonEmpty(TimeRange(0_cs, 10_cs), { Timed<int>(1_cs, 2_cs, 1) });
	EXPECT_FALSE(nonEmpty.empty());
	EXPECT_THAT(nonEmpty, Not(IsEmpty()));
}

TEST(BoundedTimeline, getRange) {
	TimeRange range(0_cs, 10_cs);
	BoundedTimeline<int> empty(range);
	EXPECT_EQ(range, empty.getRange());

	BoundedTimeline<int> nonEmpty(range, { Timed<int>(1_cs, 2_cs, 1) });
	EXPECT_EQ(range, nonEmpty.getRange());
}

TEST(BoundedTimeline, setAndClear) {
	TimeRange range(0_cs, 10_cs);
	BoundedTimeline<int> timeline(range);

	// Out of range
	timeline.set(-10_cs, -1_cs, 1);
	timeline.set(TimeRange(-5_cs, -1_cs), 2);
	timeline.set(Timed<int>(10_cs, 15_cs, 3));

	// Overlapping
	timeline.set(-2_cs, 5_cs, 4);
	timeline.set(TimeRange(-1_cs, 1_cs), 5);
	timeline.set(Timed<int>(8_cs, 12_cs, 6));

	// Within
	timeline.set(5_cs, 9_cs, 7);
	timeline.set(TimeRange(6_cs, 7_cs), 8);
	timeline.set(Timed<int>(7_cs, 8_cs, 9));

	auto expected = {
		Timed<int>(0_cs, 1_cs, 5),
		Timed<int>(1_cs, 5_cs, 4),
		Timed<int>(5_cs, 6_cs, 7),
		Timed<int>(6_cs, 7_cs, 8),
		Timed<int>(7_cs, 8_cs, 9),
		Timed<int>(8_cs, 9_cs, 7),
		Timed<int>(9_cs, 10_cs, 6)
	};
	EXPECT_THAT(timeline, ElementsAreArray(expected));
}

TEST(BoundedTimeline, shift) {
	BoundedTimeline<int> timeline(TimeRange(0_cs, 10_cs), { { 1_cs, 2_cs, 1 }, { 2_cs, 5_cs, 2 }, { 7_cs, 9_cs, 3 } });
	BoundedTimeline<int> expected(TimeRange(2_cs, 12_cs), { { 3_cs, 4_cs, 1 }, { 4_cs, 7_cs, 2 }, { 9_cs, 11_cs, 3 } });
	timeline.shift(2_cs);
	EXPECT_EQ(expected, timeline);
}

TEST(BoundedTimeline, equality) {
	vector<BoundedTimeline<int>> timelines = {
		BoundedTimeline<int>(TimeRange(0_cs, 10_cs)),
		BoundedTimeline<int>(TimeRange(0_cs, 10_cs), { { 1_cs, 2_cs, 1 } }),
		BoundedTimeline<int>(TimeRange(1_cs, 10_cs), { { 1_cs, 2_cs, 1 } })
	};

	for (size_t i = 0; i < timelines.size(); ++i) {
		for (size_t j = 0; j < timelines.size(); ++j) {
			if (i == j) {
				EXPECT_EQ(timelines[i], BoundedTimeline<int>(timelines[j])) << "i: " << i << ", j: " << j;
			} else {
				EXPECT_NE(timelines[i], timelines[j]) << "i: " << i << ", j: " << j;
			}
		}
	}
}
