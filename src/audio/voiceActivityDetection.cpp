#include "voiceActivityDetection.h"
#include <audio/DCOffset.h>
#include <audio/SampleRateConverter.h>
#include <boost/optional/optional.hpp>
#include <logging.h>
#include <pairs.h>
#include <boost/range/adaptor/transformed.hpp>
#include <stringTools.h>

using std::numeric_limits;
using std::vector;
using boost::optional;
using boost::adaptors::transformed;
using fmt::format;

float getRMS(AudioStream& audioStream, int maxSampleCount = numeric_limits<int>::max()) {
	double sum = 0; // Use double to prevent rounding errors with large number of summands
	int sampleCount;
	for (sampleCount = 0; sampleCount < maxSampleCount && !audioStream.endOfStream(); sampleCount++) {
		sum += std::pow(static_cast<double>(audioStream.readSample()), 2);
	}
	return sampleCount > 0 ? static_cast<float>(std::sqrt(sum / sampleCount)) : 0.0f;
}

float getRMS(const vector<float>& rmsSegments) {
	if (rmsSegments.empty()) return 0;

	double sum = 0; // Use double to prevent rounding errors with large number of summands
	for (float rmsSegment : rmsSegments) {
		sum += rmsSegment;
	}
	return static_cast<float>(std::sqrt(sum / rmsSegments.size()));
}

BoundedTimeline<void> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream, ProgressSink& progressSink) {
	// Make sure audio stream has no DC offset
	audioStream = removeDCOffset(std::move(audioStream));

	// Resample to remove noise
	constexpr int maxFrequency = 1000;
	constexpr int sampleRate = 2 * maxFrequency;
	audioStream = convertSampleRate(std::move(audioStream), sampleRate);

	// Collect RMS data
	vector<float> rmsSegments;
	logging::debug("RMS calculation -- start");
	int64_t centisecondCount = (audioStream->getSampleCount() - audioStream->getSampleIndex()) / 100;
	for (int cs = 0; cs < centisecondCount; ++cs) {
		rmsSegments.push_back(getRMS(*audioStream, sampleRate / 100));
		progressSink.reportProgress(static_cast<double>(cs) / centisecondCount);
	}
	logging::debug("RMS calculation -- end");

	const float rms = getRMS(rmsSegments);
	logging::debugFormat("RMS value: {0:.5f}", rms);

	// Detect activity
	const float cutoff = rms / 25;
	logging::debugFormat("RMS cutoff for voice activity detection: {0:.5f}", cutoff);
	BoundedTimeline<void> activity(audioStream->getTruncatedRange());
	for (centiseconds time = centiseconds::zero(); static_cast<size_t>(time.count()) < rmsSegments.size(); ++time) {
		float currentRMS = rmsSegments[time.count()];
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

	logging::debugFormat("Found {} sections of voice activity: {}", activity.size(),
		join(activity | transformed([](const Timed<void>& t) { return format("{0}-{1}", t.getStart(), t.getEnd()); }), ", "));

	return activity;
}
