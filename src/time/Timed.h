#pragma once

#include "time/TimeRange.h"
#include <iostream>

template<typename TValue>
class Timed {
public:
	Timed(TimeRange::time_type start, TimeRange::time_type end, const TValue& value) :
		Timed(TimeRange(start, end), value)
	{}

	Timed(const TimeRange& timeRange, const TValue& value) :
		timeRange(timeRange),
		value(value)
	{}

	Timed(const Timed&) = default;
	Timed(Timed&&) = default;

	Timed& operator=(const Timed&) = default;
	Timed& operator=(Timed&&) = default;

	TimeRange& getTimeRange() {
		return timeRange;
	}

	const TimeRange& getTimeRange() const {
		return timeRange;
	}

	TimeRange::time_type getStart() const {
		return timeRange.getStart();
	}

	TimeRange::time_type getEnd() const {
		return timeRange.getEnd();
	}

	TimeRange::time_type getDuration() const {
		return timeRange.getDuration();
	}

	void setTimeRange(const TimeRange& timeRange) {
		this->timeRange = timeRange;
	}

	TValue& getValue() {
		return value;
	}

	const TValue& getValue() const {
		return value;
	}

	void setValue(const TValue& value) {
		this->value = value;
	}

	bool operator==(const Timed& rhs) const {
		return timeRange == rhs.timeRange && value == rhs.value;
	}

	bool operator!=(const Timed& rhs) const {
		return !operator==(rhs);
	}

private:
	TimeRange timeRange;
	TValue value;
};

template<typename T>
std::ostream& operator<<(std::ostream& stream, const Timed<T>& timedValue) {
	return stream << "Timed(" << timedValue.getStart() << ", " << timedValue.getEnd() << ", " << timedValue.getValue() << ")";
}

template<>
class Timed<void> {
public:
	Timed(TimeRange::time_type start, TimeRange::time_type end) :
		Timed(TimeRange(start, end))
	{}

	Timed(const TimeRange& timeRange) :
		timeRange(timeRange)
	{}

	Timed(const Timed&) = default;
	Timed(Timed&&) = default;

	Timed& operator=(const Timed&) = default;
	Timed& operator=(Timed&&) = default;

	TimeRange& getTimeRange() {
		return timeRange;
	}

	const TimeRange& getTimeRange() const {
		return timeRange;
	}

	TimeRange::time_type getStart() const {
		return timeRange.getStart();
	}

	TimeRange::time_type getEnd() const {
		return timeRange.getEnd();
	}

	TimeRange::time_type getDuration() const {
		return timeRange.getDuration();
	}

	void setTimeRange(const TimeRange& timeRange) {
		this->timeRange = timeRange;
	}

	bool operator==(const Timed& rhs) const {
		return timeRange == rhs.timeRange;
	}

	bool operator!=(const Timed& rhs) const {
		return !operator==(rhs);
	}

private:
	TimeRange timeRange;
};

template<>
inline std::ostream& operator<<(std::ostream& stream, const Timed<void>& timedValue) {
	return stream << "Timed<void>(" << timedValue.getTimeRange().getStart() << ", " << timedValue.getTimeRange().getEnd() << ")";
}
