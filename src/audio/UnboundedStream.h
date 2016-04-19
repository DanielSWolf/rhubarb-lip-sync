#pragma once

#include "AudioStream.h"
#include <boost/optional/optional.hpp>

// Stream wrapper that allows reading before the start and past the end of the input stream.
class UnboundedStream : public AudioStream {
public:
	UnboundedStream(std::unique_ptr<AudioStream> inputStream);
	UnboundedStream(const UnboundedStream& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) const override;
	int getSampleRate() const override;
	int getSampleCount() const override;
	int getSampleIndex() const override;
	void seek(int sampleIndex) override;
	float readSample() override;

private:
	std::unique_ptr<AudioStream> innerStream;
	int sampleIndex;
	boost::optional<float> firstSample, lastSample;
};
