#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "phoneExtraction.h"
#include "audio/SampleRateConverter.h"
#include "platformTools.h"
#include "tools.h"
#include <format.h>
#include <s3types.h>
#include <regex>
#include <gsl_util.h>
#include <logging.h>
#include <audio/DCOffset.h>
#include <Timeline.h>

extern "C" {
#include <pocketsphinx.h>
#include <sphinxbase/err.h>
#include <ps_alignment.h>
#include <state_align_search.h>
#include <pocketsphinx_internal.h>
}

using std::runtime_error;
using std::invalid_argument;
using std::unique_ptr;
using std::shared_ptr;
using std::string;
using std::vector;
using std::map;
using boost::filesystem::path;
using std::function;
using std::regex;
using std::regex_replace;
using std::chrono::duration;

constexpr int sphinxSampleRate = 16000;

lambda_unique_ptr<cmd_ln_t> createConfig(path sphinxModelDirectory) {
	lambda_unique_ptr<cmd_ln_t> config(
		cmd_ln_init(
			nullptr, ps_args(), true,
			// Set acoustic model
			"-hmm", (sphinxModelDirectory / "acoustic-model").string().c_str(),
			// Set language model
			"-lm", (sphinxModelDirectory / "en-us.lm.bin").string().c_str(),
			// Set pronounciation dictionary
			"-dict", (sphinxModelDirectory / "cmudict-en-us.dict").string().c_str(),
			// Add noise against zero silence (see http://cmusphinx.sourceforge.net/wiki/faq#qwhy_my_accuracy_is_poor)
			"-dither", "yes",
			// Allow for long pauses in speech
			"-vad_prespeech", "3000",
			"-vad_postspeech", "3000",
			nullptr),
		[](cmd_ln_t* config) { cmd_ln_free_r(config); });
	if (!config) throw runtime_error("Error creating configuration.");

	return config;
}

lambda_unique_ptr<ps_decoder_t> createSpeechRecognizer(cmd_ln_t& config) {
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

void processAudioStream(AudioStream& audioStream16kHz, function<void(const vector<int16_t>&)> processBuffer, ProgressSink& progressSink) {
	// Process entire sound file
	vector<int16_t> buffer;
	const int capacity = 1600; // 0.1 second capacity
	buffer.reserve(capacity);
	int sampleCount = 0;
	do {
		// Read to buffer
		buffer.clear();
		while (buffer.size() < capacity && !audioStream16kHz.endOfStream()) {
			// Read sample
			float floatSample = audioStream16kHz.readSample();
			int16_t sample = floatSampleToInt16(floatSample);
			buffer.push_back(sample);
		}

		// Process buffer
		processBuffer(buffer);

		sampleCount += buffer.size();
		progressSink.reportProgress(static_cast<double>(sampleCount) / audioStream16kHz.getSampleCount());
	} while (buffer.size());
}

logging::Level ConvertSphinxErrorLevel(err_lvl_t errorLevel) {
	switch (errorLevel) {
	case ERR_DEBUG:
	case ERR_INFO:
	case ERR_INFOCONT:
		return logging::Level::Trace;
	case ERR_WARN:
		return logging::Level::Warn;
	case ERR_ERROR:
		return logging::Level::Error;
	case ERR_FATAL:
		return logging::Level::Fatal;
	default:
		throw invalid_argument("Unknown log level.");
	}
}

void sphinxLogCallback(void* user_data, err_lvl_t errorLevel, const char* format, ...) {
	UNUSED(user_data);

	// Create varArgs list
	va_list args;
	va_start(args, format);
	auto _ = gsl::finally([&args]() { va_end(args); });

	// Format message
	const int initialSize = 256;
	vector<char> chars(initialSize);
	bool success = false;
	while (!success) {
		int charsWritten = vsnprintf(chars.data(), chars.size(), format, args);
		if (charsWritten < 0) throw runtime_error("Error formatting Pocketsphinx log message.");

		success = charsWritten < static_cast<int>(chars.size());
		if (!success) chars.resize(chars.size() * 2);
	}
	string message(chars.data());
	boost::algorithm::trim(message);

	logging::Level logLevel = ConvertSphinxErrorLevel(errorLevel);
	logging::log(logLevel, message);
}

vector<string> recognizeWords(unique_ptr<AudioStream> audioStream, ps_decoder_t& recognizer, ProgressSink& progressSink) {
	// Convert audio stream to the exact format PocketSphinx requires
	audioStream = convertSampleRate(std::move(audioStream), sphinxSampleRate);

	// Start recognition
	int error = ps_start_utt(&recognizer);
	if (error) throw runtime_error("Error starting utterance processing for word recognition.");

	// Process entire sound file
	auto processBuffer = [&recognizer](const vector<int16_t>& buffer) {
		int searchedFrameCount = ps_process_raw(&recognizer, buffer.data(), buffer.size(), false, false);
		if (searchedFrameCount < 0) throw runtime_error("Error analyzing raw audio data for word recognition.");
	};
	processAudioStream(*audioStream.get(), processBuffer, progressSink);

	// End recognition
	error = ps_end_utt(&recognizer);
	if (error) throw runtime_error("Error ending utterance processing for word recognition.");

	// Collect words
	vector<string> result;
	int32_t score;
	for (ps_seg_t* it = ps_seg_iter(&recognizer, &score); it; it = ps_seg_next(it)) {
		const char* word = ps_seg_word(it);
		result.push_back(word);

		int firstFrame, lastFrame;
		ps_seg_frames(it, &firstFrame, &lastFrame);
		logging::logTimedEvent("word", centiseconds(firstFrame), centiseconds(lastFrame + 1), word);
	}

	return result;
}

// Splits dialog into words, doing minimal preprocessing.
// A robust solution should use TTS logic to cope with numbers, abbreviations, unknown words etc.
vector<string> extractDialogWords(string dialog) {
	// Convert to lower case
	boost::algorithm::to_lower(dialog);

	// Insert silences where appropriate
	dialog = regex_replace(dialog, regex("[,;.:!?] |-"), " <sil> ");

	// Remove all undesired characters
	dialog = regex_replace(dialog, regex("[^a-z.'\\0-9<>]"), " ");

	// Collapse whitespace
	dialog = regex_replace(dialog, regex("\\s+"), " ");

	// Trim
	boost::algorithm::trim(dialog);

	// Ugly hack: Remove trailing period
	if (boost::algorithm::ends_with(dialog, ".")) {
		dialog.pop_back();
	}

	// Split into words
	vector<string> result;
	boost::algorithm::split(result, dialog, boost::is_space());
	return result;
}

vector<s3wid_t> getWordIds(const vector<string>& words, dict_t& dictionary) {
	vector<s3wid_t> result;
	for (const string& word : words) {
		s3wid_t wordId = dict_wordid(&dictionary, word.c_str());
		if (wordId == BAD_S3WID) throw invalid_argument(fmt::format("Unknown word '{}'.", word));
		result.push_back(wordId);
	}
	return result;
}

Timeline<Phone> getPhoneAlignment(const vector<s3wid_t>& wordIds, unique_ptr<AudioStream> audioStream, ps_decoder_t& recognizer, ProgressSink& progressSink) {
	// Create alignment list
	lambda_unique_ptr<ps_alignment_t> alignment(
		ps_alignment_init(recognizer.d2p),
		[](ps_alignment_t* alignment) { ps_alignment_free(alignment); });
	if (!alignment) throw runtime_error("Error creating alignment.");
	for (s3wid_t wordId : wordIds) {
		// Add word. Initial value for duration is ignored.
		ps_alignment_add_word(alignment.get(), wordId, 0);
	}
	int error = ps_alignment_populate(alignment.get());
	if (error) throw runtime_error("Error populating alignment struct.");

	// Convert audio stream to the exact format PocketSphinx requires
	audioStream = convertSampleRate(std::move(audioStream), sphinxSampleRate);

	// Create search structure
	acmod_t* acousticModel = recognizer.acmod;
	lambda_unique_ptr<ps_search_t> search(
		state_align_search_init("state_align", recognizer.config, acousticModel, alignment.get()),
		[](ps_search_t* search) { ps_search_free(search); });
	if (!search) throw runtime_error("Error creating search.");

	// Start recognition
	error = acmod_start_utt(acousticModel);
	if (error) throw runtime_error("Error starting utterance processing for alignment.");

	// Start search
	ps_search_start(search.get());

	// Process entire sound file
	auto processBuffer = [&recognizer, &acousticModel, &search](const vector<int16_t>& buffer) {
		const int16* nextSample = buffer.data();
		size_t remainingSamples = buffer.size();
		while (acmod_process_raw(acousticModel, &nextSample, &remainingSamples, false) > 0) {
			while (acousticModel->n_feat_frame > 0) {
				ps_search_step(search.get(), acousticModel->output_frame);
				acmod_advance(acousticModel);
			}
		}
	};
	processAudioStream(*audioStream.get(), processBuffer, progressSink);

	// End search
	ps_search_finish(search.get());

	// End recognition
	acmod_end_utt(acousticModel);

	// Extract phones with timestamps
	char** phoneNames = recognizer.dict->mdef->ciname;
	Timeline<Phone> result(audioStream->getTruncatedRange());
	for (ps_alignment_iter_t* it = ps_alignment_phones(alignment.get()); it; it = ps_alignment_iter_next(it)) {
		// Get phone
		ps_alignment_entry_t* phoneEntry = ps_alignment_iter_get(it);
		s3cipid_t phoneId = phoneEntry->id.pid.cipid;
		char* phoneName = phoneNames[phoneId];

		// Add entry
		centiseconds start(phoneEntry->start);
		centiseconds duration(phoneEntry->duration);
		Timed<Phone> timedPhone(start, start + duration, PhoneConverter::get().parse(phoneName));
		result.set(timedPhone);

		logging::logTimedEvent("phone", timedPhone);
	}
	return result;
}

Timeline<Phone> detectPhones(
	unique_ptr<AudioStream> audioStream,
	boost::optional<std::string> dialog,
	ProgressSink& progressSink)
{
	// Pocketsphinx doesn't like empty input
	if (audioStream->getTruncatedRange().getLength() == centiseconds::zero()) {
		return Timeline<Phone>{};
	}

	// Discard Pocketsphinx output
	err_set_logfp(nullptr);

	// Redirect Pocketsphinx output to log
	err_set_callback(sphinxLogCallback, nullptr);

	// Make sure audio stream has no DC offset
	audioStream = removeDCOffset(std::move(audioStream));

	try {
		// Create PocketSphinx configuration
		path sphinxModelDirectory(getBinDirectory() / "res/sphinx");
		auto config = createConfig(sphinxModelDirectory);

		// Create speech recognizer
		auto recognizer = createSpeechRecognizer(*config.get());

		ProgressMerger progressMerger(progressSink);
		ProgressSink& wordRecognitionProgressSink = progressMerger.addSink(dialog ? 0.0 : 1.0);
		ProgressSink& alignmentProgressSink = progressMerger.addSink(0.5);

		// Get words
		vector<string> words = dialog
			? extractDialogWords(*dialog)
			: recognizeWords(audioStream->clone(true), *recognizer.get(), wordRecognitionProgressSink);

		// Look up words in dictionary
		vector<s3wid_t> wordIds = getWordIds(words, *recognizer->dict);

		// Align the word's phones with speech
		Timeline<Phone> result = getPhoneAlignment(wordIds, std::move(audioStream), *recognizer.get(), alignmentProgressSink);
		return result;
	}
	catch (...) {
		std::throw_with_nested(runtime_error("Error performing speech recognition via Pocketsphinx."));
	}
}
