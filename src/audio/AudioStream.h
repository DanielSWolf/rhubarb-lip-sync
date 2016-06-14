#pragma once

#include <memory>
#include "TimeRange.h"

// A mono stream of floating-point samples.
class AudioStream {
public:
	virtual ~AudioStream() {}
	virtual std::unique_ptr<AudioStream> clone(bool reset) const = 0;
	virtual int getSampleRate() const = 0;
	virtual int64_t getSampleCount() const = 0;
	TimeRange getTruncatedRange() const;
	virtual int64_t getSampleIndex() const = 0;
	virtual void seek(int64_t sampleIndex) = 0;
	bool endOfStream() const;
	virtual float readSample() = 0;
};
