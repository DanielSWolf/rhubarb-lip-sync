#include <gmock/gmock.h>
#include "tools/pairs.h"

using namespace testing;
using std::vector;
using std::initializer_list;
using std::pair;

TEST(getPairs, emptyCollection) {
	EXPECT_THAT(getPairs(vector<int>()), IsEmpty());
}

TEST(getPairs, oneElementCollection) {
	EXPECT_THAT(getPairs(vector<int>{1}), IsEmpty());
}

TEST(getPairs, validCollection) {
	{
		auto actual = getPairs(vector<int>{ 1, 2 });
		vector<pair<int, int>> expected{ {1, 2} };
		EXPECT_THAT(actual, ElementsAreArray(expected));
	}
	{
		auto actual = getPairs(vector<int>{ 1, 2, 3 });
		vector<pair<int, int>> expected{ {1, 2}, {2, 3} };
		EXPECT_THAT(actual, ElementsAreArray(expected));
	}
	{
		auto actual = getPairs(vector<int>{ 1, 2, 3, 4 });
		vector<pair<int, int>> expected{ {1, 2}, {2, 3}, {3, 4} };
		EXPECT_THAT(actual, ElementsAreArray(expected));
	}
}

