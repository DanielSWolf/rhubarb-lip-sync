#include "AudioStream.h"

TimeRange AudioStream::getTruncatedRange() const {
	return TimeRange(0cs, centiseconds(100 * getSampleCount() / getSampleRate()));
}

bool AudioStream::endOfStream() const {
	return getSampleIndex() >= getSampleCount();
}
