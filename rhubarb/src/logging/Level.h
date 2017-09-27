#pragma once

#include "tools/EnumConverter.h"

namespace logging {

	enum class Level {
		Trace,
		Debug,
		Info,
		Warn,
		Error,
		Fatal,
		EndSentinel
	};

	class LevelConverter : public EnumConverter<Level> {
	public:
		static LevelConverter& get();
	protected:
		std::string getTypeName() override;
		member_data getMemberData() override;
	};

	std::ostream& operator<<(std::ostream& stream, Level value);

	std::istream& operator >> (std::istream& stream, Level& value);

}
