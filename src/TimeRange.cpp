#include "TimeRange.h"
#include <stdexcept>
#include <ostream>
#include <format.h>

using time_type = TimeRange::time_type;

TimeRange TimeRange::zero() {
	static TimeRange zero(time_type::zero(), time_type::zero());
	return zero;
}

TimeRange::TimeRange(time_type start, time_type end) :
	start(start),
	end(end)
{
	if (start > end) {
		throw std::invalid_argument(fmt::format("Time range start must not be less than end. Start: {0}, end: {1}", start, end));
	}
}

time_type TimeRange::getStart() const {
	return start;
}

time_type TimeRange::getEnd() const {
	return end;
}

time_type TimeRange::getDuration() const {
	return end - start;
}

bool TimeRange::empty() const {
	return start == end;
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

void TimeRange::trim(const TimeRange& limits) {
	TimeRange newRange(std::max(start, limits.start), std::min(end, limits.end));
	resize(newRange);
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
