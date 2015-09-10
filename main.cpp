#include <pocketsphinx.h>
#include <stdexcept>
#include <fstream>
#include <memory>

using std::runtime_error;
using std::shared_ptr;

#define MODELDIR "X:/dev/projects/LipSync/lib/pocketsphinx/model"

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

	shared_ptr<FILE> file(
		fopen("X:/dev/projects/LipSync/lib/pocketsphinx/test/data/goforward.raw", "rb"),
		[](FILE* file) { fclose(file); });
	if (!file) throw runtime_error("Error opening sound file.");

	int error = ps_start_utt(recognizer.get());
	if (error) throw runtime_error("Error starting utterance processing.");

	int16 buffer[512];
	while (!feof(file.get())) {
		size_t sampleCount = fread(buffer, 2, 512, file.get());
		int searchedFrameCount = ps_process_raw(recognizer.get(), buffer, sampleCount, false, false);
		if (searchedFrameCount < 0) throw runtime_error("Error decoding raw audio data.");
	}
	error = ps_end_utt(recognizer.get());
	if (error) throw runtime_error("Error ending utterance processing.");

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