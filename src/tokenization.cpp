#include "tokenization.h"
#include "tools.h"
#include "ascii.h"
#include <regex>

extern "C" {
#include <cst_utt_utils.h>
#include <lang/usenglish/usenglish.h>
#include <lang/cmulex/cmu_lex.h>
}

using std::runtime_error;
using std::u32string;
using std::string;
using std::vector;
using std::regex;
using std::pair;

lambda_unique_ptr<cst_voice> createDummyVoice() {
	lambda_unique_ptr<cst_voice> voice(new_voice(), [](cst_voice* voice) { delete_voice(voice); });
	voice->name = "dummy_voice";
	usenglish_init(voice.get());
	cst_lexicon *lexicon = cmu_lex_init();
	feat_set(voice->features, "lexicon", lexicon_val(lexicon));
	return voice;
}

static const cst_synth_module synth_method_normalize[] = {
	{ "tokenizer_func", default_tokenization },		// split text into tokens
	{ "textanalysis_func", default_textanalysis },	// transform tokens into words
	{ nullptr, nullptr }
};

vector<string> tokenizeViaFlite(const string& text) {
	// Create utterance object with text
	lambda_unique_ptr<cst_utterance> utterance(new_utterance(), [](cst_utterance* utterance) { delete_utterance(utterance); });
	utt_set_input_text(utterance.get(), text.c_str());
	lambda_unique_ptr<cst_voice> voice = createDummyVoice();
	utt_init(utterance.get(), voice.get());

	// Perform tokenization and text normalization
	if (!apply_synth_method(utterance.get(), synth_method_normalize)) {
		throw runtime_error("Error normalizing text using Flite.");
	}

	vector<string> result;
	for (cst_item* item = relation_head(utt_relation(utterance.get(), "Word")); item; item = item_next(item)) {
		const char* word = item_feat_string(item, "name");
		result.push_back(word);
	}
	return result;
}

vector<string> tokenizeText(const u32string& text) {
	vector<string> words = tokenizeViaFlite(toASCII(text));

	// Join words separated by apostophes
	for (int i = words.size() - 1; i > 0; --i) {
		if (words[i].size() > 0 && words[i][0] == '\'') {
			words[i - 1].append(words[i]);
			words.erase(words.begin() + i);
		}
	}

	// Turn some symbols into words, remove the rest
	vector<pair<regex, string>> replacements {
		{ regex("&"), "and" },
		{ regex("\\*"), "times" },
		{ regex("\\+"), "plus" },
		{ regex("="), "equals" },
		{ regex("@"), "at" },
		{ regex("[^a-z']"), "" }
	};
	for (size_t i = 0; i < words.size(); ++i) {
		for (const auto& replacement : replacements) {
			words[i] = std::regex_replace(words[i], replacement.first, replacement.second);
		}
	}

	// Remove empty words
	words.erase(std::remove_if(words.begin(), words.end(), [](const string& s) { return s.empty(); }), words.end());

	return words;
}
