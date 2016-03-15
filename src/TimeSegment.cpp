#include "TimeSegment.h"

TimeSegment::TimeSegment(centiseconds start, centiseconds end) :
	start(start),
	end(end)
{}

centiseconds TimeSegment::getStart() const {
	return start;
}

centiseconds TimeSegment::getEnd() const {
	return end;
}

centiseconds TimeSegment::getLength() const {
	return end - start;
}
