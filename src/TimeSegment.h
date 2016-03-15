#pragma once
#include "centiseconds.h"

class TimeSegment {
public:
	TimeSegment(centiseconds start, centiseconds end);
	centiseconds getStart() const;
	centiseconds getEnd() const;
	centiseconds getLength() const;

private:
	const centiseconds start, end;
};
