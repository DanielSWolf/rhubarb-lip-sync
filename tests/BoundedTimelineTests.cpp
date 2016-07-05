#include <gmock/gmock.h>
#include "BoundedTimeline.h"

using namespace testing;
using std::vector;
using boost::optional;
using std::initializer_list;

TEST(BoundedTimeline, constructors_initializeState) {
	TimeRange range(-5cs, 55cs);
	auto args = {
		Timed<int>(-10cs, 30cs, 1),
		Timed<int>(10cs, 40cs, 2),
		Timed<int>(50cs, 60cs, 3)
	};
	auto expected = {
		Timed<int>(-5cs, 10cs, 1),
		Timed<int>(10cs, 40cs, 2),
		Timed<int>(50cs, 55cs, 3)
	};
	EXPECT_THAT(
		BoundedTimeline<int>(range, args.begin(), args.end()),
		ElementsAreArray(expected)
	);
	EXPECT_THAT(
		BoundedTimeline<int>(range, args),
		ElementsAreArray(expected)
	);
}

TEST(BoundedTimeline, empty) {
	BoundedTimeline<int> empty(TimeRange(0cs, 10cs));
	EXPECT_TRUE(empty.empty());
	EXPECT_THAT(empty, IsEmpty());

	BoundedTimeline<int> nonEmpty(TimeRange(0cs, 10cs), { Timed<int>(1cs, 2cs, 1) });
	EXPECT_FALSE(nonEmpty.empty());
	EXPECT_THAT(nonEmpty, Not(IsEmpty()));
}

TEST(BoundedTimeline, getRange) {
	TimeRange range(0cs, 10cs);
	BoundedTimeline<int> empty(range);
	EXPECT_EQ(range, empty.getRange());

	BoundedTimeline<int> nonEmpty(range, { Timed<int>(1cs, 2cs, 1) });
	EXPECT_EQ(range, nonEmpty.getRange());
}

TEST(BoundedTimeline, setAndClear) {
	TimeRange range(0cs, 10cs);
	BoundedTimeline<int> timeline(range);

	// Out of range
	timeline.set(-10cs, -1cs, 1);
	timeline.set(TimeRange(-5cs, -1cs), 2);
	timeline.set(Timed<int>(10cs, 15cs, 3));

	// Overlapping
	timeline.set(-2cs, 5cs, 4);
	timeline.set(TimeRange(-1cs, 1cs), 5);
	timeline.set(Timed<int>(8cs, 12cs, 6));

	// Within
	timeline.set(5cs, 9cs, 7);
	timeline.set(TimeRange(6cs, 7cs), 8);
	timeline.set(Timed<int>(7cs, 8cs, 9));

	auto expected = {
		Timed<int>(0cs, 1cs, 5),
		Timed<int>(1cs, 5cs, 4),
		Timed<int>(5cs, 6cs, 7),
		Timed<int>(6cs, 7cs, 8),
		Timed<int>(7cs, 8cs, 9),
		Timed<int>(8cs, 9cs, 7),
		Timed<int>(9cs, 10cs, 6)
	};
	EXPECT_THAT(timeline, ElementsAreArray(expected));
}

TEST(BoundedTimeline, shift) {
	BoundedTimeline<int> timeline(TimeRange(0cs, 10cs), { { 1cs, 2cs, 1 }, { 2cs, 5cs, 2 }, { 7cs, 9cs, 3 } });
	BoundedTimeline<int> expected(TimeRange(2cs, 12cs), { { 3cs, 4cs, 1 }, { 4cs, 7cs, 2 }, { 9cs, 11cs, 3 } });
	timeline.shift(2cs);
	EXPECT_EQ(expected, timeline);
}

TEST(BoundedTimeline, equality) {
	vector<BoundedTimeline<int>> timelines = {
		BoundedTimeline<int>(TimeRange(0cs, 10cs)),
		BoundedTimeline<int>(TimeRange(0cs, 10cs), { { 1cs, 2cs, 1 } }),
		BoundedTimeline<int>(TimeRange(1cs, 10cs), { { 1cs, 2cs, 1 } })
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
