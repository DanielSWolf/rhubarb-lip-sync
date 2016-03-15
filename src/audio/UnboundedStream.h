#pragma once

#include "AudioStream.h"
#include <boost/optional/optional.hpp>

// Stream wrapper that allows reading before the start and past the end of the input stream.
class UnboundedStream : public AudioStream {
public:
	UnboundedStream(std::unique_ptr<AudioStream> inputStream);
	UnboundedStream(const UnboundedStream& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) override;
	int getSampleRate() override;
	int getSampleCount() override;
	int getSampleIndex() override;
	void seek(int sampleIndex) override;
	float readSample() override;

private:
	std::unique_ptr<AudioStream> innerStream;
	int sampleIndex;
	boost::optional<float> firstSample, lastSample;
};
