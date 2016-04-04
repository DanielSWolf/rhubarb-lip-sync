#pragma once

#include <TimeRange.h>
#include <iostream>

template<typename TValue>
class Timed : public TimeRange {
public:
	Timed(time_type start, time_type end, TValue value) :
		TimeRange(start, end),
		value(value)
	{}

	Timed(TimeRange timeRange, TValue value) :
		TimeRange(timeRange),
		value(value)
	{}

	Timed(const Timed&) = default;
	Timed(Timed&&) = default;

	Timed& operator=(const Timed&) = default;
	Timed& operator=(Timed&&) = default;

	TValue& getValue() {
		return value;
	}

	const TValue& getValue() const {
		return value;
	}

	void setValue(TValue value) {
		this->value = value;
	}

	bool operator==(const Timed& rhs) const {
		return TimeRange::operator==(rhs) && value == rhs.value;
	}

	bool operator!=(const Timed& rhs) const {
		return !operator==(rhs);
	}

private:
	TValue value;
};

template<typename T>
std::ostream& operator<<(std::ostream& stream, const Timed<T>& timedValue) {
	return stream << "Timed(" << timedValue.getStart() << ", " << timedValue.getEnd() << ", " << timedValue.getValue() << ")";
}
