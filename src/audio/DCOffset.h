#pragma once

#include "AudioStream.h"

// Applies a constant DC offset to an audio stream and reduces its amplitude
// to prevent clipping
class DCOffset : public AudioStream {
public:
	DCOffset(std::unique_ptr<AudioStream> inputStream, float offset);
	DCOffset(const DCOffset& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) const override;
	int getSampleRate() const override;
	int64_t getSampleCount() const override;
	int64_t getSampleIndex() const override;
	void seek(int64_t sampleIndex) override;
	float readSample() override;

private:
	std::unique_ptr<AudioStream> inputStream;
	float offset;
	float factor;
};

std::unique_ptr<AudioStream> addDCOffset(std::unique_ptr<AudioStream> audioStream, float offset, float epsilon = 1.0f / 15000);

std::unique_ptr<AudioStream> removeDCOffset(std::unique_ptr<AudioStream> audioStream);
