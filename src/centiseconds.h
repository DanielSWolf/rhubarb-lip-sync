#pragma once

#include <chrono>

typedef std::chrono::duration<int, std::centi> centiseconds;

std::ostream& operator <<(std::ostream& stream, const centiseconds cs);
