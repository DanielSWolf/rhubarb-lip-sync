#pragma once

#include <chrono>
#include <ostream>

using centiseconds = std::chrono::duration<int, std::centi>;

// Needs to be in the same namespace as std::chrono::duration, or googletest won't pick it up.
// See https://github.com/google/googletest/blob/master/docs/advanced.md#user-content-teaching-googletest-how-to-print-your-values
namespace std {

	std::ostream& operator <<(std::ostream&, centiseconds cs);
	
}

#pragma warning(push)
#pragma warning(disable: 4455)
inline constexpr centiseconds operator "" _cs(unsigned long long cs) {
	return centiseconds(cs);
}
#pragma warning(pop)
