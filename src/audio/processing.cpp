#include "processing.h"

using std::function;
using std::vector;

// Converts a float in the range -1..1 to a signed 16-bit int
inline int16_t floatSampleToInt16(float sample) {
	sample = std::max(sample, -1.0f);
	sample = std::min(sample, 1.0f);
	return static_cast<int16_t>(((sample + 1) / 2) * (INT16_MAX - INT16_MIN) + INT16_MIN);
}

void process16bitAudioStream(AudioStream& audioStream, function<void(const vector<int16_t>&)> processBuffer, size_t bufferCapacity, ProgressSink& progressSink) {
	// Process entire sound stream
	vector<int16_t> buffer;
	buffer.reserve(bufferCapacity);
	int sampleCount = 0;
	do {
		// Read to buffer
		buffer.clear();
		while (buffer.size() < bufferCapacity && !audioStream.endOfStream()) {
			// Read sample
			float floatSample = audioStream.readSample();
			int16_t sample = floatSampleToInt16(floatSample);
			buffer.push_back(sample);
		}

		// Process buffer
		processBuffer(buffer);

		sampleCount += buffer.size();
		progressSink.reportProgress(static_cast<double>(sampleCount) / audioStream.getSampleCount());
	} while (buffer.size());
}

void process16bitAudioStream(AudioStream& audioStream, function<void(const vector<int16_t>&)> processBuffer, ProgressSink& progressSink) {
	const size_t capacity = 1600; // 0.1 second capacity
	process16bitAudioStream(audioStream, processBuffer, capacity, progressSink);
}

