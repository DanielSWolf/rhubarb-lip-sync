#pragma once

#include "AudioClip.h"

class AudioSegment : public AudioClip {
public:
	AudioSegment(std::unique_ptr<AudioClip> inputClip, const TimeRange& range);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override;
	size_type size() const override;

private:
	SampleReader createUnsafeSampleReader() const override;

	std::shared_ptr<AudioClip> inputClip;
	size_type sampleOffset, sampleCount;
};

inline int AudioSegment::getSampleRate() const {
	return inputClip->getSampleRate();
}

inline AudioClip::size_type AudioSegment::size() const {
	return sampleCount;
}

AudioEffect segment(const TimeRange& range);
