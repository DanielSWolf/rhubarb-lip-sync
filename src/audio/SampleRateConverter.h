#pragma once

#include <memory>
#include "AudioClip.h"

class SampleRateConverter : public AudioClip {
public:
	SampleRateConverter(std::unique_ptr<AudioClip> inputClip, int outputSampleRate);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override;
	size_type size() const override;
private:
	SampleReader createUnsafeSampleReader() const override;

	std::shared_ptr<AudioClip> inputClip;
	double downscalingFactor; // input sample rate / output sample rate
	int outputSampleRate;
	int64_t outputSampleCount;
};

AudioEffect resample(int sampleRate);

inline int SampleRateConverter::getSampleRate() const {
	return outputSampleRate;
}

inline AudioClip::size_type SampleRateConverter::size() const {
	return outputSampleCount;
}
