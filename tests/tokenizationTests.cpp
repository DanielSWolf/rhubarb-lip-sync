#include <gmock/gmock.h>
#include "tokenization.h"
#include <regex>
#include <unordered_set>

using namespace testing;
using std::string;
using std::u32string;
using std::vector;
using std::regex;

bool returnTrue(const string&) {
	return true;
}

TEST(tokenizeText, simpleCases) {
	EXPECT_THAT(tokenizeText(U"", returnTrue), IsEmpty());
	EXPECT_THAT(tokenizeText(U"  \t\n\r\n ", returnTrue), IsEmpty());
	EXPECT_THAT(
		tokenizeText(U"Wit is educated insolence.", returnTrue),
		ElementsAre("wit", "is", "educated", "insolence")
	);
}

TEST(tokenizeText, numbers) {
	EXPECT_THAT(
		tokenizeText(U"Henry V died at 36.", returnTrue),
		ElementsAre("henry", "the", "fifth", "died", "at", "thirty", "six")
	);
	EXPECT_THAT(
		tokenizeText(U"I spent $4.50 on gum.", returnTrue),
		ElementsAre("i", "spent", "four", "dollars", "fifty", "cents", "on", "gum")
	);
	EXPECT_THAT(
		tokenizeText(U"I was born in 1982.", returnTrue),
		ElementsAre("i", "was", "born", "in", "nineteen", "eighty", "two")
	);
}

TEST(tokenizeText, abbreviations) {
	EXPECT_THAT(
		tokenizeText(U"Prof. Foo lives on Dr. Dolittle Dr.", [](const string& word) { return word == "prof."; }),
		ElementsAre("prof.", "foo", "lives", "on", "doctor", "dolittle", "drive")
	);
}

TEST(tokenizeText, apostrophes) {
	EXPECT_THAT(
		tokenizeText(U"'Tis said he'd wish'd for a 'bus 'cause he wouldn't walk.", [](const string& word) { return word == "wouldn't"; }),
		ElementsAreArray(vector<string>{ "tis", "said", "he'd", "wish'd", "for", "a", "bus", "cause", "he", "wouldn't", "walk" })
	);
}

TEST(tokenizeText, math) {
	EXPECT_THAT(
		tokenizeText(U"'1+2*3=7", returnTrue),
		ElementsAre("one", "plus", "two", "times", "three", "equals", "seven")
	);
}

// Checks that each word contains only the characters a-z and the apostrophe
TEST(tokenizeText, wordsUseLimitedCharacters) {
	// Create string containing lots of undesirable characters
	u32string input = U"A naïve man called 晨 was having piña colada and crème brûlée.";
	for (char32_t c = 0; c <= 1000; ++c) {
		input.append(U" ");
		input.append(1, c);
	}

	regex legal("^[a-z']+$");
	auto words = tokenizeText(input, returnTrue);
	for (const string& word : words) {
		EXPECT_TRUE(std::regex_match(word, legal)) << word;
	}
}
