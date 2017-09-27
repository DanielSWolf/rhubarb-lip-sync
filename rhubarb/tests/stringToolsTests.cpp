#include <gmock/gmock.h>
#include "tools/stringTools.h"

using namespace testing;
using std::string;
using std::wstring;

// splitIntoLines

TEST(splitIntoLines, splitsOnLineBreaks) {
	EXPECT_THAT(splitIntoLines("this\nis\r\na\r\ntest"), ElementsAre("this", "is", "a", "test"));
}

TEST(splitIntoLines, handlesEmptyElements) {
	EXPECT_THAT(splitIntoLines("\n1\n\n\n2\n"), ElementsAre("", "1", "", "", "2", ""));
	EXPECT_THAT(splitIntoLines("\n"), ElementsAre("", ""));
	EXPECT_THAT(splitIntoLines(""), ElementsAre(""));
}

// wrapSingleLineString

TEST(wrapSingleLineString, basic) {
	const char* lipsum = "Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna aliqua.";
	EXPECT_THAT(wrapSingleLineString(lipsum, 30), ElementsAre("Lorem ipsum dolor sit amet,", "consectetur adipisici elit,", "sed eiusmod tempor incidunt ut", "labore et dolore magna aliqua."));
}

TEST(wrapSingleLineString, preciseWrapPosition) {
	const char* test = "a b c";
	EXPECT_THAT(wrapSingleLineString(test, 5), ElementsAre("a b c"));
	EXPECT_THAT(wrapSingleLineString(test, 4), ElementsAre("a b", "c"));
	EXPECT_THAT(wrapSingleLineString(test, 3), ElementsAre("a b", "c"));
	EXPECT_THAT(wrapSingleLineString(test, 2), ElementsAre("a", "b", "c"));
	EXPECT_THAT(wrapSingleLineString(test, 1), ElementsAre("a", "b", "c"));
}

TEST(wrapSingleLineString, overlongLines) {
	EXPECT_THAT(wrapSingleLineString("aaa bbbb ccc", 3), ElementsAre("aaa", "bbb", "b", "ccc"));
	EXPECT_THAT(wrapSingleLineString("aaa bbbb c", 3), ElementsAre("aaa", "bbb", "b c"));
	EXPECT_THAT(wrapSingleLineString("a bbbb c", 5), ElementsAre("a", "bbbb", "c"));
	EXPECT_THAT(wrapSingleLineString("aa b", 1), ElementsAre("a", "a", "b"));
}

TEST(wrapSingleLineString, discardingSpacesAtWrapPositionsAndEnd) {
	const char* test = "  a  b  c  ";
	EXPECT_THAT(wrapSingleLineString(test, 20), ElementsAre("  a  b  c"));
	EXPECT_THAT(wrapSingleLineString(test, 6), ElementsAre("  a  b", "c"));
	EXPECT_THAT(wrapSingleLineString(test, 5), ElementsAre("  a", "b  c"));
	EXPECT_THAT(wrapSingleLineString(test, 4), ElementsAre("  a", "b  c"));
	EXPECT_THAT(wrapSingleLineString(test, 3), ElementsAre("  a", "b", "c"));
	EXPECT_THAT(wrapSingleLineString(test, 2), ElementsAre("", "a", "b", "c"));
}

TEST(wrapSingleLineString, errorHandling) {
	EXPECT_NO_THROW(wrapSingleLineString("test", 1));
	EXPECT_NO_THROW(wrapSingleLineString("", 1));

	// Throw if lineLength < 1
	EXPECT_ANY_THROW(wrapSingleLineString("test", 0));
	EXPECT_ANY_THROW(wrapSingleLineString("", 0));
	EXPECT_ANY_THROW(wrapSingleLineString("test", -1));
	EXPECT_ANY_THROW(wrapSingleLineString("", -1));

	// Throw if string contains tabs
	EXPECT_ANY_THROW(wrapSingleLineString("a\tb", 10));

	// Throw if string contains line breaks
	EXPECT_ANY_THROW(wrapSingleLineString("a\nb", 10));
}

// wrapString

TEST(wrapString, basic) {
	EXPECT_THAT(wrapString("\n\nLine no 3\n\nLine no 4\n", 8), ElementsAre("", "", "Line no", "3", "", "Line no", "4", ""));
}

// latin1ToWide

TEST(latin1ToWide, basic) {
	string pangramLatin1 = "D\350s No\353l o\371 un z\351phyr ha\357 me v\352t de gla\347ons w\374rmiens, je d\356ne d'exquis r\364tis de boeuf au kir \340 l'a\377 d'\342ge m\373r & c\346tera!";
	wstring pangramWide = L"Dès Noël où un zéphyr haï me vêt de glaçons würmiens, je dîne d'exquis rôtis de boeuf au kir à l'aÿ d'âge mûr & cætera!";
	EXPECT_EQ(pangramWide, latin1ToWide(pangramLatin1));
}

// utf8ToAscii

TEST(utf8ToAscii, string) {
	EXPECT_EQ(
		"A naive man called  was having pina colada and creme brulee.",
		utf8ToAscii("A naïve man called 晨 was having piña colada and crème brûlée."));
	EXPECT_EQ(string(""), utf8ToAscii(""));
	EXPECT_EQ(string("- - - - - - - - - -"), utf8ToAscii("- ‐ ‑ ‒ – — ― ﹘ ﹣ －"));
	EXPECT_EQ(string("' ' ' ' \" \" \" \" \" \""), utf8ToAscii("‘ ’ ‚ ‛ “ ” „ ‟ « »"));
	EXPECT_EQ(string("1 2 3"), utf8ToAscii("¹ ² ³"));
	EXPECT_EQ(string("1/4 1/2 3/4"), utf8ToAscii("¼ ½ ¾"));
}
