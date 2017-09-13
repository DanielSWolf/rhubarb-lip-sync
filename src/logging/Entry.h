#pragma once

#include "Level.h"

namespace logging {
	
	struct Entry {
		Entry(Level level, const std::string& message);
		virtual ~Entry() = default;

		time_t timestamp;
		int threadCounter;
		Level level;
		std::string message;
	};

}
