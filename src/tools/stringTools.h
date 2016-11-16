#pragma once

#include <vector>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

std::vector<std::string> splitIntoLines(const std::string& s);

std::vector<std::string> wrapSingleLineString(const std::string& s, int lineLength, int hangingIndent = 0);

std::vector<std::string> wrapString(const std::string& s, int lineLength, int hangingIndent = 0);

std::wstring latin1ToWide(const std::string& s);

boost::optional<char> toAscii(char32_t ch);

std::string toAscii(const std::u32string& s);

std::u32string utf8ToUtf32(const std::string& s);

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
