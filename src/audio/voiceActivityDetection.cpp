#include "voiceActivityDetection.h"
#include <audio/DCOffset.h>
#include <audio/SampleRateConverter.h>
#include <boost/optional/optional.hpp>
#include <logging.h>
#include <pairs.h>

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

BoundedTimeline<void> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream) {
	// Make sure audio stream has no DC offset
	audioStream = removeDCOffset(std::move(audioStream));

	// Resample to remove noise
	constexpr int maxFrequency = 1000;
	constexpr int sampleRate = 2 * maxFrequency;
	audioStream = convertSampleRate(std::move(audioStream), sampleRate);

	// Detect activity
	const float rms = getRMS(*audioStream->clone(true));
	const float cutoff = rms / 50;
	BoundedTimeline<void> activity(audioStream->getTruncatedRange());
	for (centiseconds time = centiseconds::zero(); !audioStream->endOfStream(); ++time) {
		float currentRMS = getRMS(*audioStream, sampleRate / 100);
		bool active = currentRMS > cutoff;
		if (active) {
			activity.set(time, time + centiseconds(1));
		}
	}

	// Pad each activity to prevent cropping
	const centiseconds padding(3);
	for (const auto& element : BoundedTimeline<void>(activity)) {
		activity.set(element.getStart() - padding, element.getEnd() + padding);
	}

	// Fill small gaps in activity
	const centiseconds maxGap(5);
	for (const auto& pair : getPairs(activity)) {
		if (pair.second.getStart() - pair.first.getEnd() <= maxGap) {
			activity.set(pair.first.getEnd(), pair.second.getStart());
		}
	}

	// Log
	for (const auto& utterance : activity) {
		logging::logTimedEvent("utterance", utterance.getTimeRange(), std::string());
	}

	return activity;
}
