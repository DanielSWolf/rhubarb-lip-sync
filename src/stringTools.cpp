#include "stringTools.h"
#include <boost/algorithm/string/trim.hpp>

using std::string;
using std::vector;

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

vector<string> wrapSingleLineString(const string& s, int lineLength) {
	if (lineLength <= 0) throw std::invalid_argument("lineLength must be > 0.");
	if (s.find('\t') != std::string::npos) throw std::invalid_argument("s must not contain tabs.");
	if (s.find('\n') != std::string::npos) throw std::invalid_argument("s must not contain line breaks.");

	vector<string> lines;
	auto p = &s[0];
	auto lineBegin = p;
	auto lineEnd = p;
	auto end = p + s.size();
	// Iterate over input string
	while (p <= end) {
		// If we're at a word boundary: update safeLineEnd
		if (p == end || *p == ' ') {
			lineEnd = p;
		}

		// If we've hit lineLength or the end of the string: add a new result line
		if (p == end || p - lineBegin == lineLength) {
			if (lineEnd == lineBegin) {
				// The line contains a single word, which is too long. Split mid-word.
				lineEnd = p;
			}

			// Add trimmed line to list
			string line(lineBegin, lineEnd);
			boost::algorithm::trim_right(line);
			lines.push_back(line);

			// Resume after the last line, skipping spaces
			p = lineEnd;
			while (p != end && *p == ' ') ++p;
			lineBegin = lineEnd = p;
		}

		++p;
	}

	return lines;
}

vector<string> wrapString(const string& s, int lineLength) {
	vector<string> lines;
	for (string paragraph : splitIntoLines(s)) {
		auto paragraphLines = wrapSingleLineString(paragraph, lineLength);
		copy(paragraphLines.cbegin(), paragraphLines.cend(), back_inserter(lines));
	}

	return lines;
}

