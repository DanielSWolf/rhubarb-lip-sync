#pragma once

#include <string>
#include <vector>

std::vector<std::string> splitIntoLines(const std::string& s);

std::vector<std::string> wrapSingleLineString(const std::string& s, int lineLength, int hangingIndent = 0);

std::vector<std::string> wrapString(const std::string& s, int lineLength, int hangingIndent = 0);
