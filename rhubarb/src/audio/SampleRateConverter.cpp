#include <cmath>
#include "SampleRateConverter.h"
#include <stdexcept>
#include <format.h>

using std::invalid_argument;
using std::unique_ptr;
using std::make_unique;

SampleRateConverter::SampleRateConverter(unique_ptr<AudioClip> inputClip, int outputSampleRate) :
	inputClip(std::move(inputClip)),
	downscalingFactor(static_cast<double>(this->inputClip->getSampleRate()) / outputSampleRate),
	outputSampleRate(outputSampleRate),
	outputSampleCount(std::lround(this->inputClip->size() / downscalingFactor))
{
	if (outputSampleRate <= 0) {
		throw invalid_argument("Sample rate must be positive.");
	}
	if (this->inputClip->getSampleRate() < outputSampleRate) {
		throw invalid_argument(fmt::format("Upsampling not supported. Input sample rate must not be below {}Hz.", outputSampleRate));
	}
}

unique_ptr<AudioClip> SampleRateConverter::clone() const {
	return make_unique<SampleRateConverter>(*this);
}

float mean(double inputStart, double inputEnd, const SampleReader& read) {
	// Calculate weighted sum...
	double sum = 0;

	// ... first sample (weight <= 1)
	int64_t startIndex = static_cast<int64_t>(inputStart);
	sum += read(startIndex) * ((startIndex + 1) - inputStart);

	// ... middle samples (weight 1 each)
	int64_t endIndex = static_cast<int64_t>(inputEnd);
	for (int64_t index = startIndex + 1; index < endIndex; ++index) {
		sum += read(index);
	}

	// ... last sample (weight < 1)
	if (endIndex < inputEnd) {
		sum += read(endIndex) * (inputEnd - endIndex);
	}

	return static_cast<float>(sum / (inputEnd - inputStart));
}

SampleReader SampleRateConverter::createUnsafeSampleReader() const {
	return[read = inputClip->createSampleReader(), downscalingFactor = downscalingFactor, size = inputClip->size()](size_type index) {
		double inputStart = index * downscalingFactor;
		double inputEnd = std::min((index + 1) * downscalingFactor, static_cast<double>(size));
		return mean(inputStart, inputEnd, read);
	};
}

AudioEffect resample(int sampleRate) {
	return [sampleRate](unique_ptr<AudioClip> inputClip) {
		return make_unique<SampleRateConverter>(std::move(inputClip), sampleRate);
	};
}
