#include "TimeRange.h"
#include <stdexcept>

TimeRange::TimeRange(centiseconds start, centiseconds end) :
	start(start),
	end(end)
{
	if (start > end) throw std::invalid_argument("start must not be less than end.");
}

centiseconds TimeRange::getStart() const {
	return start;
}

centiseconds TimeRange::getEnd() const {
	return end;
}

centiseconds TimeRange::getLength() const {
	return end - start;
}
