#include "voiceActivityDetection.h"
#include <audio/DCOffset.h>
#include <audio/SampleRateConverter.h>
#include <boost/optional/optional.hpp>
#include <logging.h>

using std::numeric_limits;
using std::vector;
using boost::optional;

float getRMS(AudioStream& audioStream, int maxSampleCount = numeric_limits<int>::max()) {
	double sum = 0;
	int sampleCount;
	for (sampleCount = 0; sampleCount < maxSampleCount && !audioStream.endOfStream(); sampleCount++) {
		sum += std::pow(static_cast<double>(audioStream.readSample()), 2);
	}
	return sampleCount > 0 ? static_cast<float>(std::sqrt(sum / sampleCount)) : 0.0f;
}

Timeline<bool> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream) {
	// Make sure audio stream has no DC offset
	audioStream = removeDCOffset(std::move(audioStream));

	// Resample to remove noise
	constexpr int maxFrequency = 1000;
	constexpr int sampleRate = 2 * maxFrequency;
	audioStream = convertSampleRate(std::move(audioStream), sampleRate);

	// Detect activity
	const float rms = getRMS(*audioStream->clone(true));
	const float cutoff = rms / 50;
	Timeline<bool> activity(audioStream->getTruncatedRange());
	for (centiseconds time = centiseconds::zero(); !audioStream->endOfStream(); ++time) {
		float currentRMS = getRMS(*audioStream, sampleRate / 100);
		bool active = currentRMS > cutoff;
		if (active) {
			activity[time] = true;
		}
	}

	// Fill small gaps in activity
	const centiseconds maxGap(10);
	for (const auto& element : Timeline<bool>(activity)) {
		if (!element.getValue() && element.getLength() <= maxGap) {
			activity.set(static_cast<TimeRange>(element), true);
		}
	}

	// Log
	for (const auto& element : activity) {
		logging::logTimedEvent("utterance", static_cast<TimeRange>(element), std::string());
	}

	return activity;
}
