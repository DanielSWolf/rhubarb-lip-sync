#include <cmath>
#include "SampleRateConverter.h"
#include <stdexcept>
#include <algorithm>
#include <format.h>

using std::invalid_argument;

SampleRateConverter::SampleRateConverter(std::unique_ptr<AudioStream> inputStream, int outputSampleRate) :
	inputStream(std::move(inputStream)),
	downscalingFactor(static_cast<double>(this->inputStream->getSampleRate()) / outputSampleRate),
	outputSampleRate(outputSampleRate),
	outputSampleCount(std::lround(this->inputStream->getSampleCount() / downscalingFactor)),
	lastInputSample(0),
	lastInputSampleIndex(-1),
	nextOutputSampleIndex(0)
{
	if (outputSampleRate <= 0) {
		throw invalid_argument("Sample rate must be positive.");
	}
	if (this->inputStream->getSampleRate() < outputSampleRate) {
		throw invalid_argument(fmt::format("Upsampling not supported. Audio sample rate must not be below {}Hz.", outputSampleRate));
	}
}

SampleRateConverter::SampleRateConverter(const SampleRateConverter& rhs, bool reset) :
	SampleRateConverter(rhs.inputStream->clone(reset), rhs.outputSampleRate)
{
	nextOutputSampleIndex = reset ? 0 : rhs.nextOutputSampleIndex;
}

std::unique_ptr<AudioStream> SampleRateConverter::clone(bool reset) const {
	return std::make_unique<SampleRateConverter>(*this, reset);
}

int SampleRateConverter::getSampleRate() const {
	return outputSampleRate;
}

int SampleRateConverter::getSampleCount() const {
	return outputSampleCount;
}

int SampleRateConverter::getSampleIndex() const {
	return nextOutputSampleIndex;
}

void SampleRateConverter::seek(int sampleIndex) {
	if (sampleIndex < 0 || sampleIndex >= outputSampleCount) throw std::invalid_argument("sampleIndex out of range.");

	nextOutputSampleIndex = sampleIndex;
}

float SampleRateConverter::readSample() {
	if (nextOutputSampleIndex >= outputSampleCount) throw std::out_of_range("End of stream.");

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

std::unique_ptr<AudioStream> convertSampleRate(std::unique_ptr<AudioStream> audioStream, int sampleRate) {
	if (sampleRate == audioStream->getSampleRate()) {
		return audioStream;
	}
	return std::make_unique<SampleRateConverter>(std::move(audioStream), sampleRate);
}
