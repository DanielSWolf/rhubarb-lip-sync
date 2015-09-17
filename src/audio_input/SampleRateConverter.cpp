#include <cmath>
#include "SampleRateConverter.h"

using std::runtime_error;

SampleRateConverter::SampleRateConverter(std::unique_ptr<AudioStream> inputStream, int outputFrameRate) :
	inputStream(std::move(inputStream)),
	downscalingFactor(static_cast<double>(this->inputStream->getFrameRate()) / outputFrameRate),
	outputFrameRate(outputFrameRate),
	outputFrameCount(std::lround(this->inputStream->getFrameCount() / downscalingFactor)),
	lastInputSample(0),
	lastInputSampleIndex(-1),
	nextOutputSampleIndex(0)
{
	if (this->inputStream->getChannelCount() != 1) {
		throw runtime_error("Only mono input streams are supported.");
	}
	if (this->inputStream->getFrameRate() < outputFrameRate) {
		throw runtime_error("Upsampling not supported.");
	}
}

int SampleRateConverter::getFrameRate() {
	return outputFrameRate;
}

int SampleRateConverter::getFrameCount() {
	return outputFrameCount;
}

int SampleRateConverter::getChannelCount() {
	return 1;
}

bool SampleRateConverter::getNextSample(float &sample) {
	if (nextOutputSampleIndex >= outputFrameCount) return false;

	double start = nextOutputSampleIndex * downscalingFactor;
	double end = (nextOutputSampleIndex + 1) * downscalingFactor;

	sample = mean(start, end);
	nextOutputSampleIndex++;
	return true;
}

float SampleRateConverter::mean(double start, double end) {
	// Calculate weighted sum...
	double sum = 0;

	// ... first sample (weight <= 1)
	int startIndex = static_cast<int>(start);
	sum += getInputSample(startIndex) * ((startIndex + 1) - start);

	// ... middle samples (weight 1 each)
	int endIndex = static_cast<int>(end);
	for (int index = startIndex + 1; index < endIndex; index++) {
		sum += getInputSample(index);
	}

	// ... last sample (weight < 1)
	sum += getInputSample(endIndex) * (end - endIndex);

	return static_cast<float>(sum / (end - start));
}

float SampleRateConverter::getInputSample(int sampleIndex) {
	if (sampleIndex == lastInputSampleIndex) {
		return lastInputSample;
	}
	if (sampleIndex == lastInputSampleIndex + 1) {
		lastInputSampleIndex++;
		// Read the next sample.
		// If the input stream has no more samples (at the very end),
		// we'll just reuse the last sample we have.
		inputStream->getNextSample(lastInputSample);
		return lastInputSample;
	}

	throw runtime_error("Can only return the last sample or the one following it.");
}
