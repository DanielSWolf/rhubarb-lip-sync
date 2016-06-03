#pragma once

#include <vector>
#include <boost/optional.hpp>

std::vector<std::string> splitIntoLines(const std::string& s);

std::vector<std::string> wrapSingleLineString(const std::string& s, int lineLength, int hangingIndent = 0);

std::vector<std::string> wrapString(const std::string& s, int lineLength, int hangingIndent = 0);

std::wstring latin1ToWide(const std::string& s);

boost::optional<char> toASCII(char32_t ch);

std::string toASCII(const std::u32string& s);

std::u32string utf8ToUtf32(const std::string& s);