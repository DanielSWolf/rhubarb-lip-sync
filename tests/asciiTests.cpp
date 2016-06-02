#include <gmock/gmock.h>
#include "ascii.h"

using namespace testing;
using std::string;

TEST(toASCII, string) {
	EXPECT_EQ(
		"A naive man called  was having pina colada and creme brulee.",
		toASCII(U"A naïve man called 晨 was having piña colada and crème brûlée."));
	EXPECT_EQ(string(""), toASCII(U""));
}

