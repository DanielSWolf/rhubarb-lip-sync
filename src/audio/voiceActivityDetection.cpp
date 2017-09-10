#include "voiceActivityDetection.h"
#include "DcOffset.h"
#include "SampleRateConverter.h"
#include "logging/logging.h"
#include "tools/pairs.h"
#include <boost/range/adaptor/transformed.hpp>
#include <webrtc/common_audio/vad/include/webrtc_vad.h>
#include "processing.h"
#include <gsl_util.h>
#include "tools/parallel.h"
#include "AudioSegment.h"
#include "tools/stringTools.h"

using std::vector;
using boost::adaptors::transformed;
using fmt::format;
using std::runtime_error;
using std::unique_ptr;

JoiningBoundedTimeline<void> webRtcDetectVoiceActivity(const AudioClip& audioClip, ProgressSink& progressSink) {
	VadInst* vadHandle = WebRtcVad_Create();
	if (!vadHandle) throw runtime_error("Error creating WebRTC VAD handle.");

	auto freeHandle = gsl::finally([&]() { WebRtcVad_Free(vadHandle); });

	int error = WebRtcVad_Init(vadHandle);
	if (error) throw runtime_error("Error initializing WebRTC VAD handle.");

	const int aggressiveness = 2; // 0..3. The higher, the more is cut off.
	error = WebRtcVad_set_mode(vadHandle, aggressiveness);
	if (error) throw runtime_error("Error setting WebRTC VAD aggressiveness.");

	ProgressMerger progressMerger(progressSink);
	ProgressSink& pass1ProgressSink = progressMerger.addSink(1.0);
	ProgressSink& pass2ProgressSink = progressMerger.addSink(0.3);

	// Detect activity
	JoiningBoundedTimeline<void> activity(audioClip.getTruncatedRange());
	centiseconds time = 0_cs;
	const size_t bufferCapacity = audioClip.getSampleRate() / 100;
	auto processBuffer = [&](const vector<int16_t>& buffer) {
		// WebRTC is picky regarding buffer size
		if (buffer.size() < bufferCapacity) return;

		int result = WebRtcVad_Process(vadHandle, audioClip.getSampleRate(), buffer.data(), buffer.size()) == 1;
		if (result == -1) throw runtime_error("Error processing audio buffer using WebRTC VAD.");

		bool isActive = result != 0;
		if (isActive) {
			activity.set(time, time + 1_cs);
		}
		time += 1_cs;
	};
	process16bitAudioClip(audioClip, processBuffer, bufferCapacity, pass1ProgressSink);

	// WebRTC adapts to the audio. This means results may not be correct at the very beginning.
	// It sometimes returns false activity at the very beginning, mistaking the background noise for speech.
	// So we delete the first recognized utterance and re-process the corresponding audio segment.
	if (!activity.empty()) {
		TimeRange firstActivity = activity.begin()->getTimeRange();
		activity.clear(firstActivity);
		unique_ptr<AudioClip> streamStart = audioClip.clone() | segment(TimeRange(0_cs, firstActivity.getEnd()));
		time = 0_cs;
		process16bitAudioClip(*streamStart, processBuffer, bufferCapacity, pass2ProgressSink);
	}

	return activity;
}

JoiningBoundedTimeline<void> detectVoiceActivity(const AudioClip& inputAudioClip, int maxThreadCount, ProgressSink& progressSink) {
	// Prepare audio for VAD
	const unique_ptr<AudioClip> audioClip = inputAudioClip.clone() | resample(16000) | removeDcOffset();

	JoiningBoundedTimeline<void> activity(audioClip->getTruncatedRange());
	std::mutex activityMutex;

	// Split audio into segments and perform parallel VAD
	const int segmentCount = maxThreadCount;
	centiseconds audioDuration = audioClip->getTruncatedRange().getDuration();
	vector<TimeRange> audioSegments;
	for (int i = 0; i < segmentCount; ++i) {
		TimeRange segmentRange = TimeRange(i * audioDuration / segmentCount, (i + 1) * audioDuration / segmentCount);
		audioSegments.push_back(segmentRange);
	}
	runParallel([&](const TimeRange& segmentRange, ProgressSink& segmentProgressSink) {
		unique_ptr<AudioClip> audioSegment = audioClip->clone() | segment(segmentRange);
		JoiningBoundedTimeline<void> activitySegment = webRtcDetectVoiceActivity(*audioSegment, segmentProgressSink);

		std::lock_guard<std::mutex> lock(activityMutex);
		for (auto activityRange : activitySegment) {
			activityRange.getTimeRange().shift(segmentRange.getStart());
			activity.set(activityRange);
		}
	}, audioSegments, segmentCount, progressSink);

	// Fill small gaps in activity
	const centiseconds maxGap(5);
	for (const auto& pair : getPairs(activity)) {
		if (pair.second.getStart() - pair.first.getEnd() <= maxGap) {
			activity.set(pair.first.getEnd(), pair.second.getStart());
		}
	}

	// Shorten activities. WebRTC adds a bit of buffer at the end.
	const centiseconds tail(5);
	for (const auto& utterance : JoiningBoundedTimeline<void>(activity)) {
		if (utterance.getDuration() > tail && utterance.getEnd() < audioDuration) {
			activity.clear(utterance.getEnd() - tail, utterance.getEnd());
		}
	}

	logging::debugFormat("Found {} sections of voice activity: {}", activity.size(),
		join(activity | transformed([](const Timed<void>& t) { return format("{0}-{1}", t.getStart(), t.getEnd()); }), ", "));

	return activity;
}
