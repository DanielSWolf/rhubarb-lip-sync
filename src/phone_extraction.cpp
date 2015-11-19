#include <pocketsphinx.h>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <sphinxbase/err.h>
#include "phone_extraction.h"
#include "audio_input/SampleRateConverter.h"
#include "audio_input/ChannelDownmixer.h"
#include "platform_tools.h"
#include "tools.h"

using std::runtime_error;
using std::unique_ptr;
using std::shared_ptr;
using std::string;
using std::map;
using boost::filesystem::path;

unique_ptr<AudioStream> to16kHzMono(unique_ptr<AudioStream> stream) {
	// Downmix, if required
	if (stream->getChannelCount() != 1) {
		stream.reset(new ChannelDownmixer(std::move(stream)));
	}

	// Downsample, if required
	if (stream->getFrameRate() < 16000) {
		throw runtime_error("Audio sample rate must not be below 16kHz.");
	}
	if (stream->getFrameRate() != 16000) {
		stream.reset(new SampleRateConverter(std::move(stream), 16000));
	}

	return stream;
}

lambda_unique_ptr<cmd_ln_t> createConfig(path sphinxModelDirectory) {
	lambda_unique_ptr<cmd_ln_t> config(
		cmd_ln_init(
			nullptr, ps_args(), true,
			// Set acoustic model
			"-hmm", (sphinxModelDirectory / "acoustic_model").string().c_str(),
			// Set phonetic language model
			"-allphone", (sphinxModelDirectory / "en-us-phone.lm.bin").string().c_str(),
			"-allphone_ci", "yes",
			// The following settings are taken from http://cmusphinx.sourceforge.net/wiki/phonemerecognition
			// Set beam width applied to every frame in Viterbi search
			"-beam", "1e-20",
			// Set beam width applied to phone transitions
			"-pbeam", "1e-20",
			// Set language model probability weight
			"-lw", "2.0",
			nullptr),
		[](cmd_ln_t* config) { cmd_ln_free_r(config); });
	if (!config) throw runtime_error("Error creating configuration.");

	return config;
}

lambda_unique_ptr<ps_decoder_t> createPhoneRecognizer(cmd_ln_t& config) {
	lambda_unique_ptr<ps_decoder_t> recognizer(
		ps_init(&config),
		[](ps_decoder_t* recognizer) { ps_free(recognizer); });
	if (!recognizer) throw runtime_error("Error creating speech recognizer.");

	return recognizer;
}

// Converts a float in the range -1..1 to a signed 16-bit int
int16_t floatSampleToInt16(float sample) {
	sample = std::max(sample, -1.0f);
	sample = std::min(sample, 1.0f);
	return static_cast<int16_t>(((sample + 1) / 2) * (INT16_MAX - INT16_MIN) + INT16_MIN);
}

void processAudioStream(AudioStream& audioStream16kHzMono, ps_decoder_t& recognizer) {
	// Start recognition
	int error = ps_start_utt(&recognizer);
	if (error) throw runtime_error("Error starting utterance processing.");

	// Process entire sound file
	std::vector<int16_t> buffer;
	const int capacity = 1600; // 0.1 second capacity
	buffer.reserve(capacity);
	int sampleCount = 0;
	do {
		// Read to buffer
		buffer.clear();
		while (buffer.size() < capacity) {
			float sample;
			if (!audioStream16kHzMono.getNextSample(sample)) break;
			buffer.push_back(floatSampleToInt16(sample));
		}

		// Analyze buffer
		int searchedFrameCount = ps_process_raw(&recognizer, buffer.data(), buffer.size(), false, false);
		if (searchedFrameCount < 0) throw runtime_error("Error analyzing raw audio data.");

		sampleCount += buffer.size();
	} while (buffer.size());
	error = ps_end_utt(&recognizer);
	if (error) throw runtime_error("Error ending utterance processing.");

}

map<centiseconds, Phone> getPhones(ps_decoder_t& recognizer) {
	map<centiseconds, Phone> result;
	ps_seg_t *segmentationIter;
	int32 score;
	int endFrame;
	for (segmentationIter = ps_seg_iter(&recognizer, &score); segmentationIter; segmentationIter = ps_seg_next(segmentationIter)) {
		// Get phone
		char const *phone = ps_seg_word(segmentationIter);

		// Get timing
		int startFrame;
		ps_seg_frames(segmentationIter, &startFrame, &endFrame);

		result[centiseconds(startFrame)] = stringToPhone(phone);
	}
	// Add dummy entry past the last phone
	result[centiseconds(endFrame + 1)] = Phone::None;
	return result;
};

void sphinxErrorCallback(void* user_data, err_lvl_t errorLevel, const char* format, ...) {
	if (errorLevel < ERR_WARN) return;

	// Create varArgs list
	va_list args;
	va_start(args, format);
	auto _ = finally([&args](){ va_end(args); });

	// Format message
	const int initialSize = 256;
	std::vector<char> chars(initialSize);
	bool success = false;
	while (!success) {
		int charsWritten = vsnprintf(chars.data(), chars.size(), format, args);
		if (charsWritten < 0) throw runtime_error("Error formatting Pocketsphinx log message.");

		success = charsWritten < static_cast<int>(chars.size());
		if (!success) chars.resize(chars.size() * 2);
	}
	string message(chars.data());
	boost::algorithm::trim(message);

	// Append message to error string
	string* errorString = static_cast<string*>(user_data);
	if (errorString->size() > 0) *errorString += "\n";
	*errorString += message;
}

map<centiseconds, Phone> detectPhones(unique_ptr<AudioStream> audioStream) {
	// Discard Pocketsphinx output
	err_set_logfp(nullptr);

	// Collect all Pocketsphinx error messages in a string
	string errorMessage;
	err_set_callback(sphinxErrorCallback, &errorMessage);

	try {
		// Create PocketSphinx configuration
		path sphinxModelDirectory(getBinDirectory().parent_path() / "res/sphinx");
		auto config = createConfig(sphinxModelDirectory);

		// Create phone recognizer
		auto recognizer = createPhoneRecognizer(*config.get());

		// Convert audio stream to the exact format PocketSphinx requires
		audioStream = to16kHzMono(std::move(audioStream));

		// Process data
		processAudioStream(*audioStream.get(), *recognizer.get());

		// Collect results into map
		return getPhones(*recognizer.get());
	} catch (...) {
		std::throw_with_nested(runtime_error("Error detecting phones via Pocketsphinx. " + errorMessage));
	}
}
