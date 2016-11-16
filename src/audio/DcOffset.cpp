#include "DcOffset.h"
#include <cmath>

using std::unique_ptr;
using std::make_unique;

DcOffset::DcOffset(unique_ptr<AudioClip> inputClip, float offset) :
	inputClip(std::move(inputClip)),
	offset(offset),
	factor(1 / (1 + std::abs(offset)))
{}

unique_ptr<AudioClip> DcOffset::clone() const {
	return make_unique<DcOffset>(*this);
}

SampleReader DcOffset::createUnsafeSampleReader() const {
	return [read = inputClip->createSampleReader(), factor = factor, offset = offset](size_type index) {
		float sample = read(index);
		return sample * factor + offset;
	};
}

float getDcOffset(const AudioClip& audioClip) {
	int flatMeanSampleCount, fadingMeanSampleCount;
	int sampleRate = audioClip.getSampleRate();
	if (audioClip.size() > 4 * sampleRate) {
		// Long audio file. Average over the first 3 seconds, then fade out over the 4th.
		flatMeanSampleCount = 3 * sampleRate;
		fadingMeanSampleCount = 1 * sampleRate;
	} else {
		// Short audio file. Average over the entire duration.
		flatMeanSampleCount = static_cast<int>(audioClip.size());
		fadingMeanSampleCount = 0;
	}

	auto read = audioClip.createSampleReader();
	double sum = 0;
	for (int i = 0; i < flatMeanSampleCount; ++i) {
		sum += read(i);
	}
	for (int i = 0; i < fadingMeanSampleCount; ++i) {
		double weight = static_cast<double>(fadingMeanSampleCount - i) / fadingMeanSampleCount;
		sum += read(flatMeanSampleCount + i) * weight;
	}

	double totalWeight = flatMeanSampleCount + (fadingMeanSampleCount + 1) / 2.0;
	double offset = sum / totalWeight;
	return static_cast<float>(offset);
}

AudioEffect addDcOffset(float offset, float epsilon) {
	return [offset, epsilon](unique_ptr<AudioClip> inputClip) -> unique_ptr<AudioClip> {
		if (std::abs(offset) < epsilon) return std::move(inputClip);
		return make_unique<DcOffset>(std::move(inputClip), offset);
	};
}

AudioEffect removeDcOffset(float epsilon) {
	return [epsilon](unique_ptr<AudioClip> inputClip) {
		float offset = getDcOffset(*inputClip);
		return std::move(inputClip) | addDcOffset(-offset, epsilon);
	};
}
