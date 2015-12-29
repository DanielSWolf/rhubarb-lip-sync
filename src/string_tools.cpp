#include "string_tools.h"
#include <boost/algorithm/string/trim.hpp>

using std::string;
using std::vector;

vector<string> splitIntoLines(const string& s) {
	vector<string> lines;
	auto it = s.cbegin();
	auto lineBegin = it;
	auto end = s.cend();
	// Iterate over input string
	while (it <= end) {
		// Add a new result line when we hit a \n character or the end of the string
		if (it == end || *it == '\n') {
			string line(lineBegin, it);
			// Trim \r characters
			boost::algorithm::trim_if(line, [](char c) { return c == '\r'; });
			lines.push_back(line);
			lineBegin = it + 1;
		}
		++it;
	}
	
	return lines;
}

vector<string> wrapSingleLineString(const string& s, int lineLength) {
	if (lineLength <= 0) throw std::invalid_argument("lineLength must be > 0.");
	if (s.find('\t') != std::string::npos) throw std::invalid_argument("s must not contain tabs.");
	if (s.find('\n') != std::string::npos) throw std::invalid_argument("s must not contain line breaks.");

	vector<string> lines;
	auto it = s.cbegin();
	auto lineBegin = it;
	auto lineEnd = it;
	auto end = s.cend();
	// Iterate over input string
	while (it <= end) {
		// If we're at a word boundary: update safeLineEnd
		if (it == end || *it == ' ') {
			lineEnd = it;
		}

		// If we've hit lineLength or the end of the string: add a new result line
		if (it == end || it - lineBegin == lineLength) {
			if (lineEnd == lineBegin) {
				// The line contains a single word, which is too long. Split mid-word.
				lineEnd = it;
			}

			// Add trimmed line to list
			string line(lineBegin, lineEnd);
			boost::algorithm::trim_right(line);
			lines.push_back(line);

			// Resume after the last line, skipping spaces
			it = lineEnd;
			while (it != end && *it == ' ') ++it;
			lineBegin = lineEnd = it;
		}

		++it;
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

