#pragma once

#include "Level.h"

namespace logging {
	
	struct Entry {
		Entry(Level level, const std::string& message);

		time_t timestamp;
		int threadCounter;
		Level level;
		std::string message;
	};

}
