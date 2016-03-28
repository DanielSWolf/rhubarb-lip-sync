#pragma once

#include <TimeRange.h>

template<typename TValue>
class Timed : public TimeRange {
public:
	Timed(centiseconds start, centiseconds end, TValue value) :
		TimeRange(start, end),
		value(value)
	{}

	const TValue& getValue() const {
		return value;
	}

private:
	const TValue value;
};
