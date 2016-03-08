#include "AudioStream.h"

bool AudioStream::endOfStream() {
	return getSampleIndex() >= getSampleCount();
}
