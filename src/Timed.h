#pragma once

#include <TimeSegment.h>

template<typename TValue>
class Timed : public TimeSegment {
public:
	Timed(centiseconds start, centiseconds end, TValue value) :
		TimeSegment(start, end),
		value(value)
	{}

	const TValue& getValue() const {
		return value;
	}

private:
	const TValue value;
};
