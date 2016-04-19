#pragma once

#include <memory>
#include "AudioStream.h"

class SampleRateConverter : public AudioStream {
public:
	SampleRateConverter(std::unique_ptr<AudioStream> inputStream, int outputSampleRate);
	SampleRateConverter(const SampleRateConverter& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) const override;
	int getSampleRate() const override;
	int getSampleCount() const override;
	int getSampleIndex() const override;
	void seek(int sampleIndex) override;
	float readSample() override;
private:
	std::unique_ptr<AudioStream> inputStream;
	double downscalingFactor;					// input sample rate / output sample rate

	int outputSampleRate;
	int outputSampleCount;

	float lastInputSample;
	int lastInputSampleIndex;

	int nextOutputSampleIndex;

	float mean(double start, double end);
	float getInputSample(int sampleIndex);
};

std::unique_ptr<AudioStream> convertSampleRate(std::unique_ptr<AudioStream> audioStream, int sampleRate);