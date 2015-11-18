#include <boost/bimap.hpp>
#include "Phone.h"

using std::string;

template <typename L, typename R>
boost::bimap<L, R>
makeBimap(std::initializer_list<typename boost::bimap<L, R>::value_type> list) {
	return boost::bimap<L, R>(list.begin(), list.end());
}

boost::bimap<string, Phone> phonesByName = makeBimap<string, Phone>({
	{ "None", Phone::None },
	{ "Unknown", Phone::Unknown },
	{ "AO",	Phone::AO },	{ "AA",	Phone::AA },	{ "IY",	Phone::IY },	{ "UW",	Phone::UW },
	{ "EH",	Phone::EH },	{ "IH",	Phone::IH },	{ "UH",	Phone::UH },	{ "AH",	Phone::AH },
	{ "AE",	Phone::AE },	{ "EY",	Phone::EY },	{ "AY",	Phone::AY },	{ "OW",	Phone::OW },
	{ "AW",	Phone::AW },	{ "OY",	Phone::OY },	{ "ER",	Phone::ER },	{ "P",	Phone::P },
	{ "B",	Phone::B },		{ "T",	Phone::T },		{ "D",	Phone::D },		{ "K",	Phone::K },
	{ "G",	Phone::G },		{ "CH",	Phone::CH },	{ "JH",	Phone::JH },	{ "F",	Phone::F },
	{ "V",	Phone::V },		{ "TH",	Phone::TH },	{ "DH",	Phone::DH },	{ "S",	Phone::S },
	{ "Z",	Phone::Z },		{ "SH",	Phone::SH },	{ "ZH",	Phone::ZH },	{ "HH",	Phone::HH },
	{ "M",	Phone::M },		{ "N",	Phone::N },		{ "NG",	Phone::NG },	{ "L",	Phone::L },
	{ "R",	Phone::R },		{ "Y",	Phone::Y },		{ "W",	Phone::W },
});

Phone stringToPhone(const string& s) {
	auto it = phonesByName.left.find(s);
	return (it != phonesByName.left.end()) ? it->second : Phone::Unknown;
}

string phoneToString(Phone phone) {
	auto it = phonesByName.right.find(phone);
	return (it != phonesByName.right.end()) ? it->second : phoneToString(Phone::Unknown);
}

