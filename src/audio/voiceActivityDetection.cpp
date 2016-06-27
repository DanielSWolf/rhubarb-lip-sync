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
	centiseconds time = centiseconds::zero();
	auto processBuffer = [&](const vector<int16_t>& buffer) {
		bool isActive = WebRtcVad_Process(vadHandle, audioStream.getSampleRate(), buffer.data(), buffer.size()) == 1;
		if (isActive) {
			activity.set(time, time + centiseconds(1));
		}
		time += centiseconds(1);
	};
	const size_t bufferCapacity = audioStream.getSampleRate() / 100;
	process16bitAudioStream(audioStream, processBuffer, bufferCapacity, progressSink);

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
	ProgressMerger progressMerger(progressSink);
	for (int i = 0; i < segmentCount; ++i) {
		TimeRange segmentRange = TimeRange(i * audioLength / segmentCount, (i + 1) * audioLength / segmentCount);
		ProgressSink& segmentProgressSink = progressMerger.addSink(1.0);
		threadPool.addJob([segmentRange, &audioStream, &segmentProgressSink, &activityMutex, &activity] {
			std::unique_ptr<AudioStream> audioSegment = createSegment(audioStream->clone(false), segmentRange);
			BoundedTimeline<void> activitySegment = webRtcDetectVoiceActivity(*audioSegment, segmentProgressSink);

			std::lock_guard<std::mutex> lock(activityMutex);
			for (auto activityRange : activitySegment) {
				activityRange.getTimeRange().shift(segmentRange.getStart());
				activity.set(activityRange);
			}
		});
	}
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
