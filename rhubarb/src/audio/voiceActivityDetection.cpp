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
#include <webrtc/common_audio/vad/vad_core.h>

using std::vector;
using boost::adaptors::transformed;
using fmt::format;
using std::runtime_error;
using std::unique_ptr;

JoiningBoundedTimeline<void> detectVoiceActivity(
	const AudioClip& inputAudioClip,
	ProgressSink& progressSink
) {
	// Prepare audio for VAD
	constexpr int webRtcSamplingRate = 8000;
	const unique_ptr<AudioClip> audioClip = inputAudioClip.clone()
		| resample(webRtcSamplingRate)
		| removeDcOffset();

	VadInst* vadHandle = WebRtcVad_Create();
	if (!vadHandle) throw runtime_error("Error creating WebRTC VAD handle.");

	auto freeHandle = gsl::finally([&]() { WebRtcVad_Free(vadHandle); });

	int error = WebRtcVad_Init(vadHandle);
	if (error) throw runtime_error("Error initializing WebRTC VAD.");

	const int aggressiveness = 2; // 0..3. The higher, the more is cut off.
	error = WebRtcVad_set_mode(vadHandle, aggressiveness);
	if (error) throw runtime_error("Error setting WebRTC VAD aggressiveness.");

	// Detect activity
	JoiningBoundedTimeline<void> activity(audioClip->getTruncatedRange());
	centiseconds time = 0_cs;
	const size_t frameSize = webRtcSamplingRate / 100;
	const auto processBuffer = [&](const vector<int16_t>& buffer) {
		// WebRTC is picky regarding buffer size
		if (buffer.size() < frameSize) return;

		const int result = WebRtcVad_Process(
			vadHandle,
			webRtcSamplingRate,
			buffer.data(),
			buffer.size()
		);
		if (result == -1) throw runtime_error("Error processing audio buffer using WebRTC VAD.");

		// Ignore the result of WebRtcVad_Process, instead directly interpret the internal VAD flag.
		// The result of WebRtcVad_Process stays 1 for a number of frames after the last detected
		// activity.
		const bool isActive = reinterpret_cast<VadInstT*>(vadHandle)->vad == 1;

		if (isActive) {
			activity.set(time, time + 1_cs);
		}

		time += 1_cs;
	};
	process16bitAudioClip(*audioClip, processBuffer, frameSize, progressSink);

	// Fill small gaps in activity
	const centiseconds maxGap(10);
	for (const auto& pair : getPairs(activity)) {
		if (pair.second.getStart() - pair.first.getEnd() <= maxGap) {
			activity.set(pair.first.getEnd(), pair.second.getStart());
		}
	}

	// Discard very short segments of activity
	const centiseconds minSegmentLength(5);
	for (const auto& segment : Timeline<void>(activity)) {
		if (segment.getDuration() < minSegmentLength) {
			activity.clear(segment.getTimeRange());
		}
	}

	logging::debugFormat(
		"Found {} sections of voice activity: {}",
		activity.size(),
		join(activity | transformed([](const Timed<void>& t) {
			return format("{0}-{1}", t.getStart(), t.getEnd());
		}), ", ")
	);

	return activity;
}
