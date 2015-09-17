#include "16kHzMonoStream.h"
#include "WaveFileReader.h"
#include "ChannelDownmixer.h"
#include "SampleRateConverter.h"

using std::runtime_error;

std::unique_ptr<AudioStream> create16kHzMonoStream(std::string fileName) {
	// Create audio stream
	std::unique_ptr<AudioStream> stream(new WaveFileReader(fileName));

	// Downmix, if required
	if (stream->getChannelCount() != 1) {
		stream.reset(new ChannelDownmixer(std::move(stream)));
	}

	// Downsample, if required
	if (stream->getFrameRate() < 16000) {
		throw runtime_error("Sample rate must not be below 16kHz.");
	}
	if (stream->getFrameRate() != 16000) {
		stream.reset(new SampleRateConverter(std::move(stream), 16000));
	}

	return stream;
}
