#include "stringTools.h"
#include <boost/algorithm/string/trim.hpp>
#include <utf8.h>
#include <utf8proc.h>
#include <regex>
#include <format.h>

using std::string;
using std::wstring;
using std::u32string;
using std::vector;
using boost::optional;
using std::regex;
using std::regex_replace;

vector<string> splitIntoLines(const string& s) {
	vector<string> lines;
	auto p = &s[0];
	auto lineBegin = p;
	auto end = p + s.size();
	// Iterate over input string
	while (p <= end) {
		// Add a new result line when we hit a \n character or the end of the string
		if (p == end || *p == '\n') {
			string line(lineBegin, p);
			// Trim \r characters
			boost::algorithm::trim_if(line, [](char c) { return c == '\r'; });
			lines.push_back(line);
			lineBegin = p + 1;
		}
		++p;
	}
	
	return lines;
}

vector<string> wrapSingleLineString(const string& s, int lineLength, int hangingIndent) {
	if (lineLength <= 0) throw std::invalid_argument("lineLength must be > 0.");
	if (hangingIndent < 0) throw std::invalid_argument("hangingIndent must be >= 0.");
	if (hangingIndent >= lineLength) throw std::invalid_argument("hangingIndent must be < lineLength.");
	if (s.find('\t') != std::string::npos) throw std::invalid_argument("s must not contain tabs.");
	if (s.find('\n') != std::string::npos) throw std::invalid_argument("s must not contain line breaks.");

	vector<string> lines;
	auto p = &s[0];
	auto lineBegin = p;
	auto lineEnd = p;
	auto end = p + s.size();
	// Iterate over input string
	while (p <= end) {
		// If we're at a word boundary: update lineEnd
		if (p == end || *p == ' ' || *p == '|') {
			lineEnd = p;
		}

		// If we've hit lineLength or the end of the string: add a new result line
		int currentIndent = lines.empty() ? 0 : hangingIndent;
		if (p == end || p - lineBegin == lineLength - currentIndent) {
			if (lineEnd == lineBegin) {
				// The line contains a single word, which is too long. Split mid-word.
				lineEnd = p;
			}

			// Add trimmed line to list
			string line(lineBegin, lineEnd);
			boost::algorithm::trim_right(line);
			lines.push_back(string(currentIndent, ' ') + line);

			// Resume after the last line, skipping spaces
			p = lineEnd;
			while (p != end && *p == ' ') ++p;
			lineBegin = lineEnd = p;
		}

		++p;
	}

	return lines;
}

vector<string> wrapString(const string& s, int lineLength, int hangingIndent) {
	vector<string> lines;
	for (string paragraph : splitIntoLines(s)) {
		auto paragraphLines = wrapSingleLineString(paragraph, lineLength, hangingIndent);
		copy(paragraphLines.cbegin(), paragraphLines.cend(), back_inserter(lines));
	}

	return lines;
}

bool isValidUtf8(const string& s) {
	return utf8::is_valid(s.begin(), s.end());
}

wstring latin1ToWide(const string& s) {
	wstring result;
	for (unsigned char c : s) {
		result.append(1, c);
	}
	return result;
}

string utf8ToAscii(const string s) {
	// Normalize string, simplifying it as much as possible
	const NormalizationOptions options = NormalizationOptions::CompatibilityMode
		| NormalizationOptions::Decompose
		| NormalizationOptions::SimplifyLineBreaks
		| NormalizationOptions::SimplifyWhiteSpace
		| NormalizationOptions::StripCharacterMarkings
		| NormalizationOptions::StripIgnorableCharacters;
	string simplified = normalizeUnicode(s, options);

	// Replace common Unicode characters with ASCII equivalents
	static const vector<std::pair<regex, string>> replacements{
		{regex("«|»|“|”|„|‟"), "\""},
		{regex("‘|’|‚|‛|‹|›"), "'"},
		{regex("‐|‑|‒|⁃|⁻|₋|−|➖|–|—|―|﹘|﹣|－"), "-"},
		{regex("…|⋯"), "..."},
		{regex("•"), "*"},
		{regex("†|＋"), "+"},
		{regex("⁄|∕|⧸|／|/"), "/"},
		{regex("×"), "x"},
	};
	for (const auto& replacement : replacements) {
		simplified = regex_replace(simplified, replacement.first, replacement.second);
	}

	// Skip all non-ASCII code points, including multi-byte characters
	string result;
	for (char c : simplified) {
		const bool isAscii = (c & 0x80) == 0;
		if (isAscii) {
			result.append(1, c);
		}
	}

	return result;
}

string normalizeUnicode(const string s, NormalizationOptions options) {
	char* result;
	const utf8proc_ssize_t charCount = utf8proc_map(
		reinterpret_cast<const uint8_t*>(s.data()),
		s.length(),
		reinterpret_cast<uint8_t**>(&result),
		static_cast<utf8proc_option_t>(options));

	if (charCount < 0) {
		const utf8proc_ssize_t errorCode = charCount;
		const string message = string("Error normalizing string: ") + utf8proc_errmsg(errorCode);
		if (errorCode == UTF8PROC_ERROR_INVALIDOPTS) {
			throw std::invalid_argument(message);
		}
		throw std::runtime_error(message);
	}

	string resultString(result, charCount);
	free(result);
	return resultString;
}

string escapeJsonString(const string& s) {
	// JavaScript uses UTF-16 internally. As a result, character escaping in JSON strings is UTF-16-based.
	// Convert string to UTF-16
	std::u16string utf16String;
	utf8::utf8to16(s.begin(), s.end(), std::back_inserter(utf16String));

	string result;
	for (char16_t c : utf16String) {
		switch (c) {
		case '"':  result += "\\\""; break;
		case '\\': result += "\\\\"; break;
		case '\b': result += "\\b"; break;
		case '\f': result += "\\f"; break;
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\t': result += "\\t"; break;
		default:
		{
			bool needsEscaping = c < '\x20' || c >= 0x80;
			if (needsEscaping) {
				result += fmt::format("\\u{0:04x}", c);
			} else {
				result += static_cast<char>(c);
			}
		}
		}
	}
	return result;
}
