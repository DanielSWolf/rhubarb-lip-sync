#include <g2p.h>
#include <regex>
#include "tools/stringTools.h"
#include "logging/logging.h"

using std::vector;
using std::wstring;
using std::regex;
using std::wregex;
using std::invalid_argument;
using std::pair;

const vector<pair<wregex, wstring>>& getReplacementRules() {
	static vector<pair<wregex, wstring>> rules {
		#include "g2pRules.cpp"

		// Turn bigrams into unigrams for easier conversion
		{ wregex(L"ôw"), L"Ω" },
		{ wregex(L"öy"), L"ω" },
		{ wregex(L"@r"), L"ɝ" }
	};
	return rules;
}

Phone charToPhone(wchar_t c) {
	// For reference, see http://www.zompist.com/spell.html
	switch (c) {
		case L'ä': return Phone::EY;
		case L'â': return Phone::AE;
		case L'ë': return Phone::IY;
		case L'ê': return Phone::EH;
		case L'ï': return Phone::AY;
		case L'î': return Phone::IH;
		case L'ö': return Phone::OW;
		case L'ô': return Phone::AA; // could also be AO/AH
		case L'ü': return Phone::UW; // really Y+UW
		case L'û': return Phone::AH; // [ʌ] as in b[u]t
		case L'u': return Phone::UW;
		case L'ò': return Phone::AO;
		case L'ù': return Phone::UH;
		case L'@': return Phone::AH; // [ə] as in [a]lone
		case L'Ω': return Phone::AW;
		case L'ω': return Phone::OY;
		case L'y': return Phone::Y;
		case L'w': return Phone::W;
		case L'ɝ': return Phone::ER;
		case L'p': return Phone::P;
		case L'b': return Phone::B;
		case L't': return Phone::T;
		case L'd': return Phone::D;
		case L'g': return Phone::G;
		case L'k': return Phone::K;
		case L'm': return Phone::M;
		case L'n': return Phone::N;
		case L'ñ': return Phone::NG;
		case L'f': return Phone::F;
		case L'v': return Phone::V;
		case L'+': return Phone::TH; // also covers DH
		case L's': return Phone::S;
		case L'z': return Phone::Z;
		case L'$': return Phone::SH; // also covers ZH
		case L'ç': return Phone::CH;
		case L'j': return Phone::JH;
		case L'r': return Phone::R;
		case L'l': return Phone::L;
		case L'h': return Phone::HH;
	}
	return Phone::Noise;
}

vector<Phone> wordToPhones(const std::string& word) {
	static regex validWord("^[a-z']*$");
	if (!regex_match(word, validWord)) {
		throw invalid_argument(fmt::format("Word '{}' contains illegal characters.", word));
	}

	wstring wideWord = latin1ToWide(word);
	for (const auto& rule : getReplacementRules()) {
		const wregex& regex = rule.first;
		const wstring& replacement = rule.second;

		// Repeatedly apply rule until there is no more change
		bool changed;
		do {
			wstring tmp = regex_replace(wideWord, regex, replacement);
			changed = tmp != wideWord;
			wideWord = tmp;
		} while (changed);
	}

	// Remove duplicate phones
	vector<Phone> result;
	Phone lastPhone = Phone::Noise;
	for (wchar_t c : wideWord) {
		Phone phone = charToPhone(c);
		if (phone == Phone::Noise) {
			logging::errorFormat("G2P error determining pronunciation for '{}': Character '{}' is not a recognized phone shorthand.",
				word, static_cast<char>(c));
		}

		if (phone != lastPhone) {
			result.push_back(phone);
		}
		lastPhone = phone;
	}
	return result;
}
