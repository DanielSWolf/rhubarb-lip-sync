#include <boost/bimap.hpp>
#include "Phone.h"

using std::string;
using std::vector;
using std::tuple;
using std::make_tuple;

template <>
const string& getEnumTypeName<Phone>() {
	static const string name = "Shape";
	return name;
}

template <>
const vector<tuple<Phone, string>>& getEnumMembers<Phone>() {
	static const vector<tuple<Phone, string>> values = {
		make_tuple(Phone::None,		"None"),
		make_tuple(Phone::Unknown,	"Unknown"),
		make_tuple(Phone::AO,		"AO"),
		make_tuple(Phone::AA,		"AA"),
		make_tuple(Phone::IY,		"IY"),
		make_tuple(Phone::UW,		"UW"),
		make_tuple(Phone::EH,		"EH"),
		make_tuple(Phone::IH,		"IH"),
		make_tuple(Phone::UH,		"UH"),
		make_tuple(Phone::AH,		"AH"),
		make_tuple(Phone::AE,		"AE"),
		make_tuple(Phone::EY,		"EY"),
		make_tuple(Phone::AY,		"AY"),
		make_tuple(Phone::OW,		"OW"),
		make_tuple(Phone::AW,		"AW"),
		make_tuple(Phone::OY,		"OY"),
		make_tuple(Phone::ER,		"ER"),
		make_tuple(Phone::P,		"P"),
		make_tuple(Phone::B,		"B"),
		make_tuple(Phone::T,		"T"),
		make_tuple(Phone::D,		"D"),
		make_tuple(Phone::K,		"K"),
		make_tuple(Phone::G,		"G"),
		make_tuple(Phone::CH,		"CH"),
		make_tuple(Phone::JH,		"JH"),
		make_tuple(Phone::F,		"F"),
		make_tuple(Phone::V,		"V"),
		make_tuple(Phone::TH,		"TH"),
		make_tuple(Phone::DH,		"DH"),
		make_tuple(Phone::S,		"S"),
		make_tuple(Phone::Z,		"Z"),
		make_tuple(Phone::SH,		"SH"),
		make_tuple(Phone::ZH,		"ZH"),
		make_tuple(Phone::HH,		"HH"),
		make_tuple(Phone::M,		"M"),
		make_tuple(Phone::N,		"N"),
		make_tuple(Phone::NG,		"NG"),
		make_tuple(Phone::L,		"L"),
		make_tuple(Phone::R,		"R"),
		make_tuple(Phone::Y,		"Y"),
		make_tuple(Phone::W,		"W")
	};
	return values;
}

template<>
Phone parseEnum(const string& s) {
	if (s == "SIL") return Phone::None;
	Phone result;
	return tryParseEnum(s, result) ? result : Phone::Unknown;
}

std::ostream& operator<<(std::ostream& stream, Phone value) {
	return stream << enumToString(value);
}

std::istream& operator>>(std::istream& stream, Phone& value) {
	string name;
	stream >> name;
	value = parseEnum<Phone>(name);
	return stream;
}
