#pragma once

#include <memory>
#include "TimeRange.h"

// A mono stream of floating-point samples.
class AudioStream {
public:
	virtual ~AudioStream() {}
	virtual std::unique_ptr<AudioStream> clone(bool reset) const = 0;
	virtual int getSampleRate() const = 0;
	virtual int getSampleCount() const = 0;
	TimeRange getTruncatedRange() const;
	virtual int getSampleIndex() const = 0;
	virtual void seek(int sampleIndex) = 0;
	bool endOfStream() const;
	virtual float readSample() = 0;
};
