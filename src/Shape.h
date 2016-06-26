#pragma once

#include "EnumConverter.h"

// The classic Hanna-Barbera mouth shapes A-F phus the common supplements G-H
// For reference, see http://sunewatts.dk/lipsync/lipsync/article_02.php
// For visual examples, see https://flic.kr/s/aHsj86KR4J. Their shapes "BMP".."L" map to A..H.
enum class Shape {
	A,	// Closed mouth (silence, M, B, P)
	B,	// Clenched teeth (most vowels, m[e]n)
	C,	// Mouth slightly open (b[ir]d, s[ay], w[i]n...)
	D,	// Mouth wide open (b[u]t, m[y], sh[ou]ld...)
	E,	// h[ow]
	F,	// Pout ([o]ff, sh[ow])
	G,	// F, V
	EndSentinel
};

class ShapeConverter : public EnumConverter<Shape> {
public:
	static ShapeConverter& get();
protected:
	std::string getTypeName() override;
	member_data getMemberData() override;
};

std::ostream& operator<<(std::ostream& stream, Shape value);

std::istream& operator>>(std::istream& stream, Shape& value);
