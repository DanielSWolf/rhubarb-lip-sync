#include "Level.h"

using std::string;

namespace logging {

	LevelConverter& LevelConverter::get() {
		static LevelConverter converter;
		return converter;
	}

	string LevelConverter::getTypeName() {
		return "Level";
	}

	EnumConverter<Level>::member_data LevelConverter::getMemberData() {
		return member_data{
			{Level::Trace,		"Trace"},
			{Level::Debug,		"Debug"},
			{Level::Info,		"Info"},
			{Level::Warn,		"Warn"},
			{Level::Error,		"Error"},
			{Level::Fatal,		"Fatal"}
		};
	}

	std::ostream& operator<<(std::ostream& stream, Level value) {
		return LevelConverter::get().write(stream, value);
	}

	std::istream& operator >> (std::istream& stream, Level& value) {
		return LevelConverter::get().read(stream, value);
	}

}
