#include "voiceActivityDetection.h"
#include <audio/DCOffset.h>
#include <audio/SampleRateConverter.h>
#include <logging.h>
#include <pairs.h>
#include <boost/range/adaptor/transformed.hpp>
#include <webrtc/common_audio/vad/include/webrtc_vad.h>
#include "processing.h"
#include <gsl_util.h>
#include <ThreadPool.h>
#include "AudioStreamSegment.h"

using std::vector;
using boost::adaptors::transformed;
using fmt::format;
using std::runtime_error;
using std::unique_ptr;

BoundedTimeline<void> webRtcDetectVoiceActivity(AudioStream& audioStream, ProgressSink& progressSink) {
	VadInst* vadHandle = WebRtcVad_Create();
	if (!vadHandle) throw runtime_error("Error creating WebRTC VAD handle.");

	auto freeHandle = gsl::finally([&]() { WebRtcVad_Free(vadHandle); });

	int error = WebRtcVad_Init(vadHandle);
	if (error) throw runtime_error("Error initializing WebRTC VAD handle.");

	const int aggressiveness = 2; // 0..3. The higher, the more is cut off.
	error = WebRtcVad_set_mode(vadHandle, aggressiveness);
	if (error) throw runtime_error("Error setting WebRTC VAD aggressiveness.");

	// Detect activity
	BoundedTimeline<void> activity(audioStream.getTruncatedRange());
	centiseconds time = 0cs;
	const size_t bufferCapacity = audioStream.getSampleRate() / 100;
	auto processBuffer = [&](const vector<int16_t>& buffer) {
		// WebRTC is picky regarding buffer size
		if (buffer.size() < bufferCapacity) return;

		int result = WebRtcVad_Process(vadHandle, audioStream.getSampleRate(), buffer.data(), buffer.size()) == 1;
		if (result == -1) throw runtime_error("Error processing audio buffer using WebRTC VAD.");

		bool isActive = result != 0;
		if (isActive) {
			activity.set(time, time + 1cs);
		}
		time += 1cs;
	};
	process16bitAudioStream(*audioStream.clone(true), processBuffer, bufferCapacity, progressSink);

	// WebRTC adapts to the audio. This means results may not be correct at the very beginning.
	// It sometimes returns false activity at the very beginning, mistaking the background noise for speech.
	// So we delete the first recognized utterance and re-process the corresponding audio segment.
	if (!activity.empty()) {
		TimeRange firstActivity = activity.begin()->getTimeRange();
		activity.clear(firstActivity);
		unique_ptr<AudioStream> streamStart = createSegment(audioStream.clone(true), TimeRange(0cs, firstActivity.getEnd()));
		time = 0cs;
		process16bitAudioStream(*streamStart, processBuffer, bufferCapacity, progressSink);
	}

	return activity;
}

BoundedTimeline<void> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream, ProgressSink& progressSink) {
	// Prepare audio for VAD
	audioStream = removeDCOffset(convertSampleRate(std::move(audioStream), 16000));

	BoundedTimeline<void> activity(audioStream->getTruncatedRange());
	std::mutex activityMutex;

	// Split audio into segments and perform parallel VAD
	ThreadPool threadPool;
	int segmentCount = threadPool.getThreadCount();
	centiseconds audioLength = audioStream->getTruncatedRange().getLength();
	vector<TimeRange> audioSegments;
	for (int i = 0; i < segmentCount; ++i) {
		TimeRange segmentRange = TimeRange(i * audioLength / segmentCount, (i + 1) * audioLength / segmentCount);
		audioSegments.push_back(segmentRange);
	}
	threadPool.schedule(audioSegments, [&](const TimeRange& segmentRange, ProgressSink& segmentProgressSink) {
		unique_ptr<AudioStream> audioSegment = createSegment(audioStream->clone(false), segmentRange);
		BoundedTimeline<void> activitySegment = webRtcDetectVoiceActivity(*audioSegment, segmentProgressSink);

		std::lock_guard<std::mutex> lock(activityMutex);
		for (auto activityRange : activitySegment) {
			activityRange.getTimeRange().shift(segmentRange.getStart());
			activity.set(activityRange);
		}
	}, progressSink);
	threadPool.waitAll();

	// Fill small gaps in activity
	const centiseconds maxGap(5);
	for (const auto& pair : getPairs(activity)) {
		if (pair.second.getStart() - pair.first.getEnd() <= maxGap) {
			activity.set(pair.first.getEnd(), pair.second.getStart());
		}
	}

	// Pad each activity to give the recognizer some breathing room
	const centiseconds padding(3);
	for (const auto& element : BoundedTimeline<void>(activity)) {
		activity.set(element.getStart() - padding, element.getEnd() + padding);
	}

	logging::debugFormat("Found {} sections of voice activity: {}", activity.size(),
		join(activity | transformed([](const Timed<void>& t) { return format("{0}-{1}", t.getStart(), t.getEnd()); }), ", "));

	return activity;
}
