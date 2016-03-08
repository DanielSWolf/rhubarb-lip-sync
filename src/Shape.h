#pragma once

#include <iostream>
#include <vector>
#include "enumTools.h"

// The classic Hanna-Barbera mouth shapes A-F phus the common supplements G-H
// For reference, see http://sunewatts.dk/lipsync/lipsync/article_02.php
// For visual examples, see https://flic.kr/s/aHsj86KR4J. Their shapes "BMP".."L" map to A..H.
enum class Shape {
	Invalid = -1,
	A,	// Closed mouth (silence, M, B, P)
	B,	// Clenched teeth (most vowels, m[e]n)
	C,	// Mouth slightly open (b[ir]d, s[ay], w[i]n...)
	D,	// Mouth wide open (b[u]t, m[y], sh[ou]ld...)
	E,	// h[ow]
	F,	// Pout ([o]ff, sh[ow])
	G,	// F, V
	H	// L
};

template<>
const std::string& getEnumTypeName<Shape>();

template<>
const std::vector<std::tuple<Shape, std::string>>& getEnumMembers<Shape>();

std::ostream& operator<<(std::ostream& stream, Shape value);

std::istream& operator>>(std::istream& stream, Shape& value);
