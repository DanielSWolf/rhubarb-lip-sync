#pragma once
#include "centiseconds.h"

class TimeRange {
public:
	using time_type = centiseconds;

	TimeRange(time_type start, time_type end);
	TimeRange(const TimeRange&) = default;
	TimeRange(TimeRange&&) = default;

	TimeRange& operator=(const TimeRange&) = default;
	TimeRange& operator=(TimeRange&&) = default;

	time_type getStart() const;
	time_type getEnd() const;
	time_type getLength() const;

	void resize(const TimeRange& newRange);
	void resize(time_type start, time_type end);

	bool operator==(const TimeRange& rhs) const;
	bool operator!=(const TimeRange& rhs) const;
private:
	time_type start, end;
};

std::ostream& operator<<(std::ostream& stream, const TimeRange& timeRange);
