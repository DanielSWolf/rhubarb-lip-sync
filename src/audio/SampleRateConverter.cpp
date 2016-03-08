#include <cmath>
#include "SampleRateConverter.h"
#include <stdexcept>
#include <algorithm>

using std::runtime_error;

SampleRateConverter::SampleRateConverter(std::unique_ptr<AudioStream> inputStream, int outputFrameRate) :
	inputStream(std::move(inputStream)),
	downscalingFactor(static_cast<double>(this->inputStream->getSampleRate()) / outputFrameRate),
	outputFrameRate(outputFrameRate),
	outputFrameCount(std::lround(this->inputStream->getSampleCount() / downscalingFactor)),
	lastInputSample(0),
	lastInputSampleIndex(-1),
	nextOutputSampleIndex(0)
{
	if (this->inputStream->getSampleRate() < outputFrameRate) {
		throw runtime_error("Upsampling not supported.");
	}
}

SampleRateConverter::SampleRateConverter(const SampleRateConverter& rhs, bool reset) :
	SampleRateConverter(rhs.inputStream->clone(reset), outputFrameRate)
{
	nextOutputSampleIndex = reset ? 0 : rhs.nextOutputSampleIndex;
}

std::unique_ptr<AudioStream> SampleRateConverter::clone(bool reset) {
	return std::make_unique<SampleRateConverter>(*this, reset);
}

int SampleRateConverter::getSampleRate() {
	return outputFrameRate;
}

int SampleRateConverter::getSampleCount() {
	return outputFrameCount;
}

int SampleRateConverter::getSampleIndex() {
	return nextOutputSampleIndex;
}

void SampleRateConverter::seek(int sampleIndex) {
	if (sampleIndex < 0 || sampleIndex >= outputFrameCount) throw std::invalid_argument("sampleIndex out of range.");

	nextOutputSampleIndex = sampleIndex;
}

float SampleRateConverter::readSample() {
	if (nextOutputSampleIndex >= outputFrameCount) throw std::out_of_range("End of stream.");

	double inputStart = nextOutputSampleIndex * downscalingFactor;
	double inputEnd = (nextOutputSampleIndex + 1) * downscalingFactor;

	nextOutputSampleIndex++;
	return mean(inputStart, inputEnd);
}

float SampleRateConverter::mean(double inputStart, double inputEnd) {
	// Calculate weighted sum...
	double sum = 0;

	// ... first sample (weight <= 1)
	int startIndex = static_cast<int>(inputStart);
	sum += getInputSample(startIndex) * ((startIndex + 1) - inputStart);

	// ... middle samples (weight 1 each)
	int endIndex = static_cast<int>(inputEnd);
	for (int index = startIndex + 1; index < endIndex; index++) {
		sum += getInputSample(index);
	}

	// ... last sample (weight < 1)
	sum += getInputSample(endIndex) * (inputEnd - endIndex);

	return static_cast<float>(sum / (inputEnd - inputStart));
}

float SampleRateConverter::getInputSample(int sampleIndex) {
	sampleIndex = std::min(sampleIndex, inputStream->getSampleCount() - 1);
	if (sampleIndex < 0) return 0.0f;

	if (sampleIndex == lastInputSampleIndex) {
		return lastInputSample;
	}

	if (sampleIndex != inputStream->getSampleIndex()) {
		inputStream->seek(sampleIndex);
	}
	lastInputSample = inputStream->readSample();
	lastInputSampleIndex = sampleIndex;
	return lastInputSample;
}
