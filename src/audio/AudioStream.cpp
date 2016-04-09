#include "AudioStream.h"

TimeRange AudioStream::getTruncatedRange() {
	return TimeRange(centiseconds::zero(), centiseconds(100 * getSampleCount() / getSampleRate()));
}

bool AudioStream::endOfStream() {
	return getSampleIndex() >= getSampleCount();
}
