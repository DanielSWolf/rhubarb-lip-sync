#include <gmock/gmock.h>
#include "BoundedTimeline.h"

using namespace testing;
using cs = centiseconds;
using std::vector;
using boost::optional;
using std::initializer_list;

TEST(BoundedTimeline, constructors_initializeState) {
	TimeRange range(cs(-5), cs(55));
	auto args = {
		Timed<int>(cs(-10), cs(30), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(50), cs(60), 3)
	};
	auto expected = {
		Timed<int>(cs(-5), cs(10), 1),
		Timed<int>(cs(10), cs(40), 2),
		Timed<int>(cs(50), cs(55), 3)
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
	BoundedTimeline<int> empty(TimeRange(cs(0), cs(10)));
	EXPECT_TRUE(empty.empty());
	EXPECT_THAT(empty, IsEmpty());

	BoundedTimeline<int> nonEmpty(TimeRange(cs(0), cs(10)), { Timed<int>(cs(1), cs(2), 1) });
	EXPECT_FALSE(nonEmpty.empty());
	EXPECT_THAT(nonEmpty, Not(IsEmpty()));
}

TEST(BoundedTimeline, getRange) {
	TimeRange range(cs(0), cs(10));
	BoundedTimeline<int> empty(range);
	EXPECT_EQ(range, empty.getRange());

	BoundedTimeline<int> nonEmpty(range, { Timed<int>(cs(1), cs(2), 1) });
	EXPECT_EQ(range, nonEmpty.getRange());
}

TEST(BoundedTimeline, setAndClear) {
	TimeRange range(cs(0), cs(10));
	BoundedTimeline<int> timeline(range);

	// Out of range
	timeline.set(cs(-10), cs(-1), 1);
	timeline.set(TimeRange(cs(-5), cs(-1)), 2);
	timeline.set(Timed<int>(cs(10), cs(15), 3));

	// Overlapping
	timeline.set(cs(-2), cs(5), 4);
	timeline.set(TimeRange(cs(-1), cs(1)), 5);
	timeline.set(Timed<int>(cs(8), cs(12), 6));

	// Within
	timeline.set(cs(5), cs(9), 7);
	timeline.set(TimeRange(cs(6), cs(7)), 8);
	timeline.set(Timed<int>(cs(7), cs(8), 9));

	auto expected = {
		Timed<int>(cs(0), cs(1), 5),
		Timed<int>(cs(1), cs(5), 4),
		Timed<int>(cs(5), cs(6), 7),
		Timed<int>(cs(6), cs(7), 8),
		Timed<int>(cs(7), cs(8), 9),
		Timed<int>(cs(8), cs(9), 7),
		Timed<int>(cs(9), cs(10), 6)
	};
	EXPECT_THAT(timeline, ElementsAreArray(expected));
}

TEST(BoundedTimeline, shift) {
	BoundedTimeline<int> timeline(TimeRange(cs(0), cs(10)), { { cs(1), cs(2), 1 }, { cs(2), cs(5), 2 }, { cs(7), cs(9), 3 } });
	BoundedTimeline<int> expected(TimeRange(cs(2), cs(12)), { { cs(3), cs(4), 1 }, { cs(4), cs(7), 2 }, { cs(9), cs(11), 3 } });
	timeline.shift(cs(2));
	EXPECT_EQ(expected, timeline);
}

TEST(BoundedTimeline, equality) {
	vector<BoundedTimeline<int>> timelines = {
		BoundedTimeline<int>(TimeRange(cs(0), cs(10))),
		BoundedTimeline<int>(TimeRange(cs(0), cs(10)), { { cs(1), cs(2), 1 } }),
		BoundedTimeline<int>(TimeRange(cs(1), cs(10)), { { cs(1), cs(2), 1 } })
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
