#include "TimeRange.h"
#include <stdexcept>
#include <ostream>

using time_type = TimeRange::time_type;

TimeRange::TimeRange(time_type start, time_type end) :
	start(start),
	end(end)
{
	if (start > end) throw std::invalid_argument("Start must not be less than end.");
}

time_type TimeRange::getStart() const {
	return start;
}

time_type TimeRange::getEnd() const {
	return end;
}

time_type TimeRange::getLength() const {
	return end - start;
}

void TimeRange::resize(const TimeRange& newRange) {
	start = newRange.start;
	end = newRange.end;
}

void TimeRange::resize(time_type start, time_type end) {
	resize(TimeRange(start, end));
}

void TimeRange::shift(time_type offset) {
	start += offset;
	end += offset;
}

void TimeRange::grow(time_type value) {
	start -= value;
	end += value;
}

void TimeRange::shrink(time_type value) {
	grow(-value);
}

bool TimeRange::operator==(const TimeRange& rhs) const {
	return start == rhs.start && end == rhs.end;
}

bool TimeRange::operator!=(const TimeRange& rhs) const {
	return !operator==(rhs);
}

std::ostream& operator<<(std::ostream& stream, const TimeRange& timeRange) {
	return stream << "TimeRange(" << timeRange.getStart() << ", " << timeRange.getEnd() << ")";
}
