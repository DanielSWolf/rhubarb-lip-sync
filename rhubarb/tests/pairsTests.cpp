#include <gmock/gmock.h>
#include "tools/pairs.h"

using namespace testing;
using std::vector;
using std::pair;

TEST(getPairs, emptyCollection) {
    EXPECT_THAT(getPairs(vector<int>()), IsEmpty());
}

TEST(getPairs, oneElementCollection) {
    EXPECT_THAT(getPairs(vector<int>{1}), IsEmpty());
}

TEST(getPairs, validCollection) {
    {
        const auto actual = getPairs(vector<int> { 1, 2 });
        const vector<pair<int, int>> expected { { 1, 2 } };
        EXPECT_THAT(actual, ElementsAreArray(expected));
    }
    {
        const auto actual = getPairs(vector<int> { 1, 2, 3 });
        const vector<pair<int, int>> expected { { 1, 2 }, { 2, 3 } };
        EXPECT_THAT(actual, ElementsAreArray(expected));
    }
    {
        const auto actual = getPairs(vector<int> { 1, 2, 3, 4 });
        const vector<pair<int, int>> expected { { 1, 2 }, { 2, 3 }, { 3, 4 } };
        EXPECT_THAT(actual, ElementsAreArray(expected));
    }
}

