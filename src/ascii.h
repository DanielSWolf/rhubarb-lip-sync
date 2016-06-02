#pragma once

#include <string>
#include <boost/optional.hpp>

boost::optional<char> toASCII(char32_t ch);
std::string toASCII(const std::u32string& s);