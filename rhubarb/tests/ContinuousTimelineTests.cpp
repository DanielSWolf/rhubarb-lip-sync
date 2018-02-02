#include <gmock/gmock.h>
#include "time/ContinuousTimeline.h"

using namespace testing;
using std::vector;
using boost::optional;
using std::initializer_list;

TEST(ContinuousTimeline, constructors_initializeState) {
	TimeRange range(-5_cs, 55_cs);
	int defaultValue = -1;
	auto args = {
		Timed<int>(-10_cs, 30_cs, 1),
		Timed<int>(10_cs, 40_cs, 2),
		Timed<int>(50_cs, 60_cs, 3)
	};
	auto expected = {
		Timed<int>(-5_cs, 10_cs, 1),
		Timed<int>(10_cs, 40_cs, 2),
		Timed<int>(40_cs, 50_cs, defaultValue),
		Timed<int>(50_cs, 55_cs, 3)
	};
	EXPECT_THAT(
		ContinuousTimeline<int>(range, defaultValue, args.begin(), args.end()),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		ContinuousTimeline<int>(range, defaultValue, vector<Timed<int>>(args)),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		ContinuousTimeline<int>(range, defaultValue, args),
		ElementsAreArray(expected)
	);
}

TEST(ContinuousTimeline, empty) {
	ContinuousTimeline<int> empty(TimeRange(10_cs, 10_cs), -1);
	EXPECT_TRUE(empty.empty());
	EXPECT_THAT(empty, IsEmpty());

	ContinuousTimeline<int> nonEmpty1(TimeRange(0_cs, 10_cs), -1);
	EXPECT_FALSE(nonEmpty1.empty());
	EXPECT_THAT(nonEmpty1, Not(IsEmpty()));

	ContinuousTimeline<int> nonEmpty2(TimeRange(0_cs, 10_cs), -1, { Timed<int>(1_cs, 2_cs, 1) });
	EXPECT_FALSE(nonEmpty2.empty());
	EXPECT_THAT(nonEmpty2, Not(IsEmpty()));
}

TEST(ContinuousTimeline, setAndClear) {
	TimeRange range(0_cs, 10_cs);
	int defaultValue = -1;
	ContinuousTimeline<int> timeline(range, defaultValue);

	// Out of range
	timeline.set(-10_cs, -1_cs, 1);
	timeline.set(TimeRange(-5_cs, -1_cs), 2);
	timeline.set(Timed<int>(10_cs, 15_cs, 3));

	// Overlapping
	timeline.set(-2_cs, 3_cs, 4);
	timeline.set(TimeRange(-1_cs, 1_cs), 5);
	timeline.set(Timed<int>(8_cs, 12_cs, 6));

	// Within
	timeline.set(5_cs, 9_cs, 7);
	timeline.set(TimeRange(6_cs, 7_cs), 8);
	timeline.set(Timed<int>(7_cs, 8_cs, 9));

	auto expected = {
		Timed<int>(0_cs, 1_cs, 5),
		Timed<int>(1_cs, 3_cs, 4),
		Timed<int>(3_cs, 5_cs, defaultValue),
		Timed<int>(5_cs, 6_cs, 7),
		Timed<int>(6_cs, 7_cs, 8),
		Timed<int>(7_cs, 8_cs, 9),
		Timed<int>(8_cs, 9_cs, 7),
		Timed<int>(9_cs, 10_cs, 6)
	};
	EXPECT_THAT(timeline, ElementsAreArray(expected));
}

TEST(ContinuousTimeline, shift) {
	ContinuousTimeline<int> timeline(TimeRange(0_cs, 10_cs), -1, { { 1_cs, 2_cs, 1 },{ 2_cs, 5_cs, 2 },{ 7_cs, 9_cs, 3 } });
	ContinuousTimeline<int> expected(TimeRange(2_cs, 12_cs), -1, { { 3_cs, 4_cs, 1 },{ 4_cs, 7_cs, 2 },{ 9_cs, 11_cs, 3 } });
	timeline.shift(2_cs);
	EXPECT_EQ(expected, timeline);
}

TEST(ContinuousTimeline, equality) {
	vector<ContinuousTimeline<int>> timelines = {
		ContinuousTimeline<int>(TimeRange(0_cs, 10_cs), -1),
		ContinuousTimeline<int>(TimeRange(0_cs, 10_cs), 1),
		ContinuousTimeline<int>(TimeRange(0_cs, 10_cs), -1, { { 1_cs, 2_cs, 1 } }),
		ContinuousTimeline<int>(TimeRange(1_cs, 10_cs), -1, { { 1_cs, 2_cs, 1 } })
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
