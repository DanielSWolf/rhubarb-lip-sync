#pragma once

#include <memory>
#include <vector>
#include "AudioStream.h"

class SampleRateConverter : public AudioStream {
public:
	SampleRateConverter(std::unique_ptr<AudioStream> inputStream, int outputFrameRate);
	virtual int getFrameRate() override;
	virtual int getFrameCount() override;
	virtual int getChannelCount() override;
	virtual bool getNextSample(float &sample) override;
private:
	// The stream we're reading from
	std::unique_ptr<AudioStream> inputStream;

	// input frame rate / output frame rate
	double downscalingFactor;

	int outputFrameRate;
	int outputFrameCount;

	float lastInputSample;
	int lastInputSampleIndex;

	int nextOutputSampleIndex;

	float mean(double start, double end);
	float getInputSample(int sampleIndex);
};
