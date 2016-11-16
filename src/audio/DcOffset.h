#pragma once

#include "AudioClip.h"

// Applies a constant DC offset to an audio clip and reduces its amplitude
// to prevent clipping
class DcOffset : public AudioClip {
public:
	DcOffset(std::unique_ptr<AudioClip> inputClip, float offset);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override;
	size_type size() const override;
private:
	SampleReader createUnsafeSampleReader() const override;

	std::shared_ptr<AudioClip> inputClip;
	float offset;
	float factor;
};

inline int DcOffset::getSampleRate() const {
	return inputClip->getSampleRate();
}

inline AudioClip::size_type DcOffset::size() const {
	return inputClip->size();
}

float getDcOffset(const AudioClip& audioClip);

AudioEffect addDcOffset(float offset, float epsilon = 1.0f / 15000);
AudioEffect removeDcOffset(float epsilon = 1.0f / 15000);
