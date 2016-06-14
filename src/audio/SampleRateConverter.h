#pragma once

#include <memory>
#include "AudioStream.h"

class SampleRateConverter : public AudioStream {
public:
	SampleRateConverter(std::unique_ptr<AudioStream> inputStream, int outputSampleRate);
	SampleRateConverter(const SampleRateConverter& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) const override;
	int getSampleRate() const override;
	int64_t getSampleCount() const override;
	int64_t getSampleIndex() const override;
	void seek(int64_t sampleIndex) override;
	float readSample() override;
private:
	std::unique_ptr<AudioStream> inputStream;
	double downscalingFactor;					// input sample rate / output sample rate

	int outputSampleRate;
	int64_t outputSampleCount;

	float lastInputSample;
	int64_t lastInputSampleIndex;

	int64_t nextOutputSampleIndex;

	float mean(double start, double end);
	float getInputSample(int64_t sampleIndex);
};

std::unique_ptr<AudioStream> convertSampleRate(std::unique_ptr<AudioStream> audioStream, int sampleRate);