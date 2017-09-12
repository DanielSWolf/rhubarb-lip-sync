#include <gmock/gmock.h>
#include "recognition/g2p.h"

using namespace testing;
using std::vector;
using std::pair;
using std::string;

TEST(wordToPhones, basic) {
	EXPECT_THAT(wordToPhones(""), IsEmpty());

	EXPECT_ANY_THROW(wordToPhones("Invalid"));

	// The following phones are based on actual output, *not* ideal output.
	vector<pair<string, vector<Phone>>> words {
		{ "once", { Phone::AA, Phone::N, Phone::S }},
		{ "upon", { Phone::UW, Phone::P, Phone::AH, Phone::N }},
		{ "a", { Phone::AH }},
		{ "midnight", { Phone::M, Phone::IH, Phone::D, Phone::N, Phone::AY, Phone::T }},
		{ "dreary", { Phone::D, Phone::R, Phone::IY, Phone::R, Phone::IY }},
		{ "while", { Phone::W, Phone::AY, Phone::L }},
		{ "i", { Phone::IY }},
		{ "pondered", { Phone::P, Phone::AA, Phone::N, Phone::D, Phone::IY, Phone::R, Phone::EH, Phone::D }},
		{ "weak", { Phone::W, Phone::IY, Phone::K }},
		{ "and", { Phone::AE, Phone::N, Phone::D }},
		{ "weary", { Phone::W, Phone::IY, Phone::R, Phone::IY }}
	};
	for (const auto& word : words) {
		EXPECT_THAT(wordToPhones(word.first), ElementsAreArray(word.second)) << "Original word: '" << word.first << "'";
	}
}