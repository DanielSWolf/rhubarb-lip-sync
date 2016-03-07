#pragma once

#include <memory>

// A mono stream of floating-point samples.
class AudioStream {
public:
	virtual ~AudioStream() {}
	virtual std::unique_ptr<AudioStream> clone(bool reset) = 0;
	virtual int getSampleRate() = 0;
	virtual int getSampleCount() = 0;
	virtual int getSampleIndex() = 0;
	virtual void seek(int sampleIndex) = 0;
	bool endOfStream();
	virtual float readSample() = 0;
};
