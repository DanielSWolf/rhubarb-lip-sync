#include "stringTools.h"
#include <boost/algorithm/string/trim.hpp>
#include <codecvt>

using std::string;
using std::wstring;
using std::u32string;
using std::vector;
using boost::optional;

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

wstring latin1ToWide(const string& s) {
	wstring result;
	for (unsigned char c : s) {
		result.append(1, c);
	}
	return result;
}

optional<char> toAscii(char32_t ch) {
	switch (ch) {
#include "asciiCases.cpp"
	default:
		return ch < 0x80 ? static_cast<char>(ch) : optional<char>();
	}
}

string toAscii(const u32string& s) {
	string result;
	for (char32_t ch : s) {
		optional<char> ascii = toAscii(ch);
		if (ascii) result.append(1, *ascii);
	}
	return result;
}

u32string utf8ToUtf32(const string& s) {
#if defined(_MSC_VER) && _MSC_VER <= 1900
	// Workaround for Visual Studio 2015
	// See https://connect.microsoft.com/VisualStudio/feedback/details/1403302/unresolved-external-when-using-codecvt-utf8
	std::wstring_convert<std::codecvt_utf8<uint32_t>, uint32_t> convert;
	return u32string(reinterpret_cast<const char32_t*>(convert.from_bytes(s).c_str()));
#else
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
	return convert.from_bytes(s);
#endif
}
