#include "processing.h"
#include <algorithm>

using std::function;
using std::vector;

// Converts a float in the range -1..1 to a signed 16-bit int
inline int16_t floatSampleToInt16(float sample) {
	sample = std::max(sample, -1.0f);
	sample = std::min(sample, 1.0f);
	return static_cast<int16_t>(((sample + 1) / 2) * (INT16_MAX - INT16_MIN) + INT16_MIN);
}

void process16bitAudioClip(
	const AudioClip& audioClip,
	const function<void(const vector<int16_t>&)>& processBuffer,
	size_t bufferCapacity,
	ProgressSink& progressSink
) {
	// Process entire sound stream
	vector<int16_t> buffer;
	buffer.reserve(bufferCapacity);
	size_t sampleCount = 0;
	auto it = audioClip.begin();
	const auto end = audioClip.end();
	do {
		// Read to buffer
		buffer.clear();
		for (; buffer.size() < bufferCapacity && it != end; ++it) {
			// Read sample to buffer
			buffer.push_back(floatSampleToInt16(*it));
		}

		// Process buffer
		processBuffer(buffer);

		sampleCount += buffer.size();
		progressSink.reportProgress(static_cast<double>(sampleCount) / static_cast<double>(audioClip.size()));
	} while (!buffer.empty());
}

void process16bitAudioClip(
	const AudioClip& audioClip,
	const function<void(const vector<int16_t>&)>& processBuffer,
	ProgressSink& progressSink
) {
	const size_t capacity = 1600; // 0.1 second capacity
	process16bitAudioClip(audioClip, processBuffer, capacity, progressSink);
}

vector<int16_t> copyTo16bitBuffer(const AudioClip& audioClip) {
	vector<int16_t> result(static_cast<size_t>(audioClip.size()));
	int index = 0;
	for (float sample : audioClip) {
		result[index++] = floatSampleToInt16(sample);
	}
	return result;
}
