#pragma once

#include <chrono>

typedef std::chrono::duration<int, std::centi> centiseconds;

std::ostream& operator <<(std::ostream& stream, const centiseconds cs);

// I know user-defined literals should start with an underscore.
// But chances are slim the standard will introduce a "cs" literal
// with a different meaning than "centiseconds".
#pragma warning(push)
#pragma warning(disable: 4455)
inline constexpr centiseconds operator ""cs(unsigned long long cs) {
	return centiseconds(cs);
}
#pragma warning(pop)
