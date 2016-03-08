#pragma once

#include <memory>
#include "AudioStream.h"

class SampleRateConverter : public AudioStream {
public:
	SampleRateConverter(std::unique_ptr<AudioStream> inputStream, int outputFrameRate);
	SampleRateConverter(const SampleRateConverter& rhs, bool reset);
	std::unique_ptr<AudioStream> clone(bool reset) override;
	int getSampleRate() override;
	int getSampleCount() override;
	int getSampleIndex() override;
	void seek(int sampleIndex) override;
	float readSample() override;
private:
	std::unique_ptr<AudioStream> inputStream;
	double downscalingFactor;					// input frame rate / output frame rate

	int outputFrameRate;
	int outputFrameCount;

	float lastInputSample;
	int lastInputSampleIndex;

	int nextOutputSampleIndex;

	float mean(double start, double end);
	float getInputSample(int sampleIndex);
};
