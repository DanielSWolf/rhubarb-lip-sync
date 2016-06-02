#include "ascii.h"

using std::string;
using std::u32string;
using boost::optional;

optional<char> toASCII(char32_t ch) {
	switch (ch) {
		#include "asciiCases.cpp"
	default:
		return ch < 0x80 ? static_cast<char>(ch) : optional<char>();
	}
}

string toASCII(const u32string& s) {
	string result;
	for (char32_t ch : s) {
		optional<char> ascii = toASCII(ch);
		if (ascii) result.append(1, *ascii);
	}
	return result;
}
