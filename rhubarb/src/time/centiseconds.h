#pragma once

#include <chrono>
#include <ostream>

typedef std::chrono::duration<int, std::centi> centiseconds;

std::ostream& operator <<(std::ostream& stream, const centiseconds cs);

#pragma warning(push)
#pragma warning(disable: 4455)
inline constexpr centiseconds operator "" _cs(unsigned long long cs) {
	return centiseconds(cs);
}
#pragma warning(pop)
