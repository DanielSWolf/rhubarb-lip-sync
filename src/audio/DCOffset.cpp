#include "DCOffset.h"
#include <gsl_util.h>

DCOffset::DCOffset(std::unique_ptr<AudioStream> inputStream, float offset) :
	inputStream(std::move(inputStream)),
	offset(offset),
	factor(1 / (1 + std::abs(offset)))
{}

DCOffset::DCOffset(const DCOffset& rhs, bool reset) :
	inputStream(rhs.inputStream->clone(reset)),
	offset(rhs.offset),
	factor(rhs.factor)
{}

std::unique_ptr<AudioStream> DCOffset::clone(bool reset) {
	return std::make_unique<DCOffset>(*this, reset);
}

int DCOffset::getSampleRate() {
	return inputStream->getSampleRate();
}

int DCOffset::getSampleCount() {
	return inputStream->getSampleCount();
}

int DCOffset::getSampleIndex() {
	return inputStream->getSampleIndex();
}

void DCOffset::seek(int sampleIndex) {
	inputStream->seek(sampleIndex);
}

float DCOffset::readSample() {
	float sample = inputStream->readSample();
	return sample * factor + offset;
}

std::unique_ptr<AudioStream> addDCOffset(std::unique_ptr<AudioStream> audioStream, float offset, float epsilon) {
	if (std::abs(offset) < epsilon) return audioStream;
	return std::make_unique<DCOffset>(std::move(audioStream), offset);
}

float getDCOffset(AudioStream& audioStream) {
	int flatMeanSampleCount, fadingMeanSampleCount;
	int sampleRate = audioStream.getSampleRate();
	if (audioStream.getSampleCount() > 4 * sampleRate) {
		// Long audio file. Average over the first 3 seconds, then fade out over the 4th.
		flatMeanSampleCount = 3 * sampleRate;
		fadingMeanSampleCount = 1 * sampleRate;
	} else {
		// Short audio file. Average over the entire length.
		flatMeanSampleCount = audioStream.getSampleCount();
		fadingMeanSampleCount = 0;
	}

	int originalSampleIndex = audioStream.getSampleIndex();
	audioStream.seek(0);
	auto restorePosition = gsl::finally([&]() { audioStream.seek(originalSampleIndex); });

	double sum = 0;
	for (int i = 0; i < flatMeanSampleCount; i++) {
		sum += audioStream.readSample();
	}
	for (int i = 0; i < fadingMeanSampleCount; i++) {
		double weight = static_cast<double>(fadingMeanSampleCount - i) / fadingMeanSampleCount;
		sum += audioStream.readSample() * weight;
	}

	double totalWeight = flatMeanSampleCount + (fadingMeanSampleCount + 1) / 2.0;
	double offset = sum / totalWeight;
	return static_cast<float>(offset);
}

std::unique_ptr<AudioStream> removeDCOffset(std::unique_ptr<AudioStream> inputStream) {
	float offset = getDCOffset(*inputStream.get());
	return addDCOffset(std::move(inputStream), -offset);
}
