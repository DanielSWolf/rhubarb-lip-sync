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

vector<TimeSegment> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream) {
	// Make sure audio stream has no DC offset
	audioStream = removeDCOffset(std::move(audioStream));

	// Resample to remove noise
	constexpr int maxFrequency = 1000;
	constexpr int sampleRate = 2 * maxFrequency;
	audioStream = convertSampleRate(std::move(audioStream), sampleRate);

	float rms = getRMS(*audioStream->clone(true));
	float cutoff = rms / 50;
	centiseconds maxGap(10);

	vector<TimeSegment> result;
	optional<centiseconds> segmentStart, segmentEnd;
	for (centiseconds time = centiseconds(0); !audioStream->endOfStream(); ++time) {
		float currentPower = getRMS(*audioStream, sampleRate / 100);
		bool active = currentPower > cutoff;
		if (active) {
			if (!segmentStart) {
				segmentStart = time;
			}
			segmentEnd = time + centiseconds(1);
		} else if (segmentEnd && time > segmentEnd.value() + maxGap) {
			result.push_back(TimeSegment(segmentStart.value(), segmentEnd.value()));
			logTimedEvent("utterance", segmentStart.value(), segmentEnd.value(), "");
			segmentStart.reset();
			segmentEnd.reset();
		}
	}
	if (segmentEnd) {
		result.push_back(TimeSegment(segmentStart.value(), segmentEnd.value()));
	}

	return result;
}
