#include "voiceActivityDetection.h"
#include <audio/DCOffset.h>
#include <audio/SampleRateConverter.h>
#include <logging.h>
#include <pairs.h>
#include <boost/range/adaptor/transformed.hpp>
#include <webrtc/common_audio/vad/include/webrtc_vad.h>
#include "processing.h"
#include <gsl_util.h>

using std::vector;
using boost::adaptors::transformed;
using fmt::format;
using std::runtime_error;

BoundedTimeline<void> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream, ProgressSink& progressSink) {
	// Prepare audio for VAD
	audioStream = removeDCOffset(convertSampleRate(std::move(audioStream), 16000));

	VadInst* vadHandle = WebRtcVad_Create();
	if (!vadHandle) throw runtime_error("Error creating WebRTC VAD handle.");

	auto freeHandle = gsl::finally([&]() { WebRtcVad_Free(vadHandle); });

	int error = WebRtcVad_Init(vadHandle);
	if (error) throw runtime_error("Error initializing WebRTC VAD handle.");

	const int aggressiveness = 2; // 0..3. The higher, the more is cut off.
	error = WebRtcVad_set_mode(vadHandle, aggressiveness);
	if (error) throw runtime_error("Error setting WebRTC VAD aggressiveness.");

	// Detect activity
	BoundedTimeline<void> activity(audioStream->getTruncatedRange());
	centiseconds time = centiseconds::zero();
	auto processBuffer = [&](const vector<int16_t>& buffer) {
		bool isActive = WebRtcVad_Process(vadHandle, audioStream->getSampleRate(), buffer.data(), buffer.size()) == 1;
		if (isActive) {
			activity.set(time, time + centiseconds(1));
		}
		time += centiseconds(1);
	};
	const size_t bufferCapacity = audioStream->getSampleRate() / 100;
	process16bitAudioStream(*audioStream.get(), processBuffer, bufferCapacity, progressSink);

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
