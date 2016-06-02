#include <gmock/gmock.h>
#include "tokenization.h"
#include <regex>

using namespace testing;
using std::string;
using std::u32string;
using std::vector;
using std::regex;

TEST(tokenizeText, simpleCases) {
	EXPECT_THAT(tokenizeText(U""), IsEmpty());
	EXPECT_THAT(tokenizeText(U"  \t\n\r\n "), IsEmpty());
	EXPECT_THAT(
		tokenizeText(U"Wit is educated insolence."),
		ElementsAre("wit", "is", "educated", "insolence")
	);
}

TEST(tokenizeText, numbers) {
	EXPECT_THAT(
		tokenizeText(U"Henry V died at 36."),
		ElementsAre("henry", "the", "fifth", "died", "at", "thirty", "six")
	);
	EXPECT_THAT(
		tokenizeText(U"I spent $4.50 on gum."),
		ElementsAre("i", "spent", "four", "dollars", "fifty", "cents", "on", "gum")
	);
	EXPECT_THAT(
		tokenizeText(U"I was born in 1982."),
		ElementsAre("i", "was", "born", "in", "nineteen", "eighty", "two")
	);
}

TEST(tokenizeText, abbreviations) {
	EXPECT_THAT(
		tokenizeText(U"I live on Dr. Dolittle Dr."),
		ElementsAre("i", "live", "on", "doctor", "dolittle", "drive")
	);
}

TEST(tokenizeText, apostrophes) {
	// HACK: "wouldn't" really should not become "wouldnt"!
	EXPECT_THAT(
		tokenizeText(U"'Tis said he'd wish'd for a 'bus 'cause he wouldn't walk."),
		ElementsAreArray(vector<string>{ "tis", "said", "he'd", "wish'd", "for", "a", "bus", "cause", "he", "wouldnt", "walk" })
	);
}

TEST(tokenizeText, math) {
	EXPECT_THAT(
		tokenizeText(U"'1+2*3=7"),
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
	auto words = tokenizeText(input);
	for (const string& word : words) {
		EXPECT_TRUE(std::regex_match(word, legal)) << word;
	}
}
