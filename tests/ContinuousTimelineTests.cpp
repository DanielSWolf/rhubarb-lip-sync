#include <gmock/gmock.h>
#include "ContinuousTimeline.h"

using namespace testing;
using std::vector;
using boost::optional;
using std::initializer_list;

TEST(ContinuousTimeline, constructors_initializeState) {
	TimeRange range(-5cs, 55cs);
	int defaultValue = -1;
	auto args = {
		Timed<int>(-10cs, 30cs, 1),
		Timed<int>(10cs, 40cs, 2),
		Timed<int>(50cs, 60cs, 3)
	};
	auto expected = {
		Timed<int>(-5cs, 10cs, 1),
		Timed<int>(10cs, 40cs, 2),
		Timed<int>(40cs, 50cs, defaultValue),
		Timed<int>(50cs, 55cs, 3)
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
	ContinuousTimeline<int> empty(TimeRange(10cs, 10cs), -1);
	EXPECT_TRUE(empty.empty());
	EXPECT_THAT(empty, IsEmpty());

	ContinuousTimeline<int> nonEmpty1(TimeRange(0cs, 10cs), -1);
	EXPECT_FALSE(nonEmpty1.empty());
	EXPECT_THAT(nonEmpty1, Not(IsEmpty()));

	ContinuousTimeline<int> nonEmpty2(TimeRange(0cs, 10cs), -1, { Timed<int>(1cs, 2cs, 1) });
	EXPECT_FALSE(nonEmpty2.empty());
	EXPECT_THAT(nonEmpty2, Not(IsEmpty()));
}

TEST(ContinuousTimeline, setAndClear) {
	TimeRange range(0cs, 10cs);
	int defaultValue = -1;
	ContinuousTimeline<int> timeline(range, defaultValue);

	// Out of range
	timeline.set(-10cs, -1cs, 1);
	timeline.set(TimeRange(-5cs, -1cs), 2);
	timeline.set(Timed<int>(10cs, 15cs, 3));

	// Overlapping
	timeline.set(-2cs, 3cs, 4);
	timeline.set(TimeRange(-1cs, 1cs), 5);
	timeline.set(Timed<int>(8cs, 12cs, 6));

	// Within
	timeline.set(5cs, 9cs, 7);
	timeline.set(TimeRange(6cs, 7cs), 8);
	timeline.set(Timed<int>(7cs, 8cs, 9));

	auto expected = {
		Timed<int>(0cs, 1cs, 5),
		Timed<int>(1cs, 3cs, 4),
		Timed<int>(3cs, 5cs, defaultValue),
		Timed<int>(5cs, 6cs, 7),
		Timed<int>(6cs, 7cs, 8),
		Timed<int>(7cs, 8cs, 9),
		Timed<int>(8cs, 9cs, 7),
		Timed<int>(9cs, 10cs, 6)
	};
	EXPECT_THAT(timeline, ElementsAreArray(expected));
}

TEST(ContinuousTimeline, shift) {
	ContinuousTimeline<int> timeline(TimeRange(0cs, 10cs), -1, { { 1cs, 2cs, 1 },{ 2cs, 5cs, 2 },{ 7cs, 9cs, 3 } });
	ContinuousTimeline<int> expected(TimeRange(2cs, 12cs), -1, { { 3cs, 4cs, 1 },{ 4cs, 7cs, 2 },{ 9cs, 11cs, 3 } });
	timeline.shift(2cs);
	EXPECT_EQ(expected, timeline);
}

TEST(ContinuousTimeline, equality) {
	vector<ContinuousTimeline<int>> timelines = {
		ContinuousTimeline<int>(TimeRange(0cs, 10cs), -1),
		ContinuousTimeline<int>(TimeRange(0cs, 10cs), 1),
		ContinuousTimeline<int>(TimeRange(0cs, 10cs), -1, { { 1cs, 2cs, 1 } }),
		ContinuousTimeline<int>(TimeRange(1cs, 10cs), -1, { { 1cs, 2cs, 1 } })
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
