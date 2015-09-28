#include <pocketsphinx.h>
#include <stdexcept>
#include <fstream>
#include <memory>
#include <vector>
#include <iostream>
#include <chrono>
#include "audio_input/16kHzMonoStream.h"

using std::runtime_error;
using std::shared_ptr;
using std::unique_ptr;

#define MODELDIR "X:/dev/projects/LipSync/lib/pocketsphinx/model"

// Converts a float in the range -1..1 to a signed 16-bit int
int16_t floatSampleToInt16(float sample) {
	sample = std::max(sample, -1.0f);
	sample = std::min(sample, 1.0f);
	return static_cast<int16_t>(((sample + 1) / 2) * (INT16_MAX - INT16_MIN) + INT16_MIN);
}

int main(int argc, char *argv[]) {
	shared_ptr<cmd_ln_t> config(
		cmd_ln_init(
			nullptr, ps_args(), true,
			// Set acoustic model
			"-hmm", MODELDIR "/en-us/en-us",
			// Set phonetic language model
			"-allphone", MODELDIR "/en-us/en-us-phone.lm.bin",
			// The following settings are Voodoo to me.
			// I copied them from http://cmusphinx.sourceforge.net/wiki/phonemerecognition
			// Set beam width applied to every frame in Viterbi search
			"-beam", "1e-20",
			// Set beam width applied to phone transitions
			"-pbeam", "1e-20",
			// Set language model probability weight
			"-lw", "2.0",
			nullptr),
		[](cmd_ln_t* config) { cmd_ln_free_r(config); });
	if (!config) throw runtime_error("Error creating configuration.");

	shared_ptr<ps_decoder_t> recognizer(
		ps_init(config.get()),
		[](ps_decoder_t* recognizer) { ps_free(recognizer); });
	if (!recognizer) throw runtime_error("Error creating speech recognizer.");

	unique_ptr<AudioStream> audioStream =
		create16kHzMonoStream(R"(C:\Users\Daniel\Desktop\audio-test\test 16000Hz 1ch 16bit.wav)");

	int error = ps_start_utt(recognizer.get());
	if (error) throw runtime_error("Error starting utterance processing.");

	auto start = std::chrono::steady_clock::now();

	std::vector<int16_t> buffer;
	const int capacity = 1600;
	buffer.reserve(capacity); // 0.1 second capacity
	int sampleCount = 0;
	do {
		// Read to buffer
		buffer.clear();
		while (buffer.size() < capacity) {
			float sample;
			if (!audioStream->getNextSample(sample)) break;
			buffer.push_back(floatSampleToInt16(sample));
		}

		// Analyze buffer
		int searchedFrameCount = ps_process_raw(recognizer.get(), buffer.data(), buffer.size(), false, false);
		if (searchedFrameCount < 0) throw runtime_error("Error decoding raw audio data.");

		sampleCount += buffer.size();

		std::cout << sampleCount / 16000.0 << "s\n";
	} while (buffer.size());
	error = ps_end_utt(recognizer.get());
	if (error) throw runtime_error("Error ending utterance processing.");

	auto end = std::chrono::steady_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() << "\n";

	ps_seg_t *segmentationIter;
	int32 score;
	for (segmentationIter = ps_seg_iter(recognizer.get(), &score); segmentationIter; segmentationIter = ps_seg_next(segmentationIter)) {
		// Get phoneme
		char const *phoneme = ps_seg_word(segmentationIter);

		// Get timing
		int startFrame, endFrame;
		ps_seg_frames(segmentationIter, &startFrame, &endFrame);

		printf(">>> %-5s %-5d %-5d\n", phoneme, startFrame, endFrame);
	}

	return 0;
}