#include <boost/bimap.hpp>
#include "Phone.h"

using std::string;

PhoneConverter& PhoneConverter::get() {
	static PhoneConverter converter;
	return converter;
}

string PhoneConverter::getTypeName() {
	return "Phone";
}

EnumConverter<Phone>::member_data PhoneConverter::getMemberData() {
	return member_data{
		{ Phone::None,		"None" },
		{ Phone::Unknown,	"Unknown" },
		{ Phone::AO,		"AO" },
		{ Phone::AA,		"AA" },
		{ Phone::IY,		"IY" },
		{ Phone::UW,		"UW" },
		{ Phone::EH,		"EH" },
		{ Phone::IH,		"IH" },
		{ Phone::UH,		"UH" },
		{ Phone::AH,		"AH" },
		{ Phone::AE,		"AE" },
		{ Phone::EY,		"EY" },
		{ Phone::AY,		"AY" },
		{ Phone::OW,		"OW" },
		{ Phone::AW,		"AW" },
		{ Phone::OY,		"OY" },
		{ Phone::ER,		"ER" },
		{ Phone::P,			"P" },
		{ Phone::B,			"B" },
		{ Phone::T,			"T" },
		{ Phone::D,			"D" },
		{ Phone::K,			"K" },
		{ Phone::G,			"G" },
		{ Phone::CH,		"CH" },
		{ Phone::JH,		"JH" },
		{ Phone::F,			"F" },
		{ Phone::V,			"V" },
		{ Phone::TH,		"TH" },
		{ Phone::DH,		"DH" },
		{ Phone::S,			"S" },
		{ Phone::Z,			"Z" },
		{ Phone::SH,		"SH" },
		{ Phone::ZH,		"ZH" },
		{ Phone::HH,		"HH" },
		{ Phone::M,			"M" },
		{ Phone::N,			"N" },
		{ Phone::NG,		"NG" },
		{ Phone::L,			"L" },
		{ Phone::R,			"R" },
		{ Phone::Y,			"Y" },
		{ Phone::W,			"W" }
	};
}

boost::optional<Phone> PhoneConverter::tryParse(const string& s) {
	if (s == "SIL") return Phone::None;
	auto result = EnumConverter<Phone>::tryParse(s);
	return result ? result : Phone::Unknown;
}

std::ostream& operator<<(std::ostream& stream, Phone value) {
	return PhoneConverter::get().write(stream, value);
}

std::istream& operator>>(std::istream& stream, Phone& value) {
	return PhoneConverter::get().read(stream, value);
}
