#pragma once

#include <vector>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <utf8proc.h>

std::vector<std::string> splitIntoLines(const std::string& s);

std::vector<std::string> wrapSingleLineString(const std::string& s, int lineLength, int hangingIndent = 0);

std::vector<std::string> wrapString(const std::string& s, int lineLength, int hangingIndent = 0);

bool isValidUtf8(const std::string& s);

std::wstring latin1ToWide(const std::string& s);

boost::optional<char> toAscii(char32_t ch);

std::string utf8ToAscii(const std::string s);

enum class NormalizationOptions : int {
	CompatibilityMode = UTF8PROC_COMPAT,
	Compose = UTF8PROC_COMPOSE,
	Decompose = UTF8PROC_DECOMPOSE,
	StripIgnorableCharacters = UTF8PROC_IGNORE,
	ThrowOnUnassignedCodepoints = UTF8PROC_REJECTNA,
	SimplifyLineBreaks = UTF8PROC_NLF2LS,
	SimplifyWhiteSpace = UTF8PROC_STRIPCC,
	StripCharacterMarkings = UTF8PROC_STRIPMARK
};

constexpr NormalizationOptions
operator|(NormalizationOptions a, NormalizationOptions b) {
	return static_cast<NormalizationOptions>(static_cast<int>(a) | static_cast<int>(b));
}

std::string normalizeUnicode(const std::string s, NormalizationOptions options);

template<typename T>
std::string join(T range, const std::string separator) {
	std::string result;
	bool isFirst = true;
	for (const auto& element : range) {
		if (!isFirst) result.append(separator);
		isFirst = false;
		result.append(boost::lexical_cast<std::string>(element));
	}
	return result;
}

std::string escapeJsonString(const std::string& s);