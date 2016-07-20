#pragma once

#include "AudioClip.h"

// Applies a constant DC offset to an audio clip and reduces its amplitude
// to prevent clipping
class DCOffset : public AudioClip {
public:
	DCOffset(std::unique_ptr<AudioClip> inputClip, float offset);
	std::unique_ptr<AudioClip> clone() const override;
	int getSampleRate() const override;
	size_type size() const override;
private:
	SampleReader createUnsafeSampleReader() const override;

	std::shared_ptr<AudioClip> inputClip;
	float offset;
	float factor;
};

inline int DCOffset::getSampleRate() const {
	return inputClip->getSampleRate();
}

inline AudioClip::size_type DCOffset::size() const {
	return inputClip->size();
}

float getDCOffset(const AudioClip& audioClip);

AudioEffect addDCOffset(float offset, float epsilon = 1.0f / 15000);
AudioEffect removeDCOffset(float epsilon = 1.0f / 15000);
