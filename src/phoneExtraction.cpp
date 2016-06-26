#include <boost/filesystem.hpp>
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
#include <audio/voiceActivityDetection.h>
#include <audio/AudioStreamSegment.h>
#include "languageModels.h"
#include "tokenization.h"
#include "g2p.h"
#include "ContinuousTimeline.h"
#include "audio/processing.h"
#include "ThreadPool.h"
#include "ObjectPool.h"

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
using boost::optional;
using std::u32string;

constexpr int sphinxSampleRate = 16000;

const path& getSphinxModelDirectory() {
	static path sphinxModelDirectory(getBinDirectory() / "res/sphinx");
	return sphinxModelDirectory;
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
	regex waste("^(DEBUG|INFO|INFOCONT|WARN|ERROR|FATAL): ");
	string message = regex_replace(chars.data(), waste, "", std::regex_constants::format_first_only);
	boost::algorithm::trim(message);

	logging::Level logLevel = ConvertSphinxErrorLevel(errorLevel);
	logging::log(logLevel, message);
}

BoundedTimeline<string> recognizeWords(unique_ptr<AudioStream> audioStream, ps_decoder_t& decoder, ProgressSink& progressSink) {
	// Convert audio stream to the exact format PocketSphinx requires
	audioStream = convertSampleRate(std::move(audioStream), sphinxSampleRate);

	// Restart timing at 0
	ps_start_stream(&decoder);

	// Start recognition
	int error = ps_start_utt(&decoder);
	if (error) throw runtime_error("Error starting utterance processing for word recognition.");

	// Process entire sound stream
	auto processBuffer = [&decoder](const vector<int16_t>& buffer) {
		int searchedFrameCount = ps_process_raw(&decoder, buffer.data(), buffer.size(), false, false);
		if (searchedFrameCount < 0) throw runtime_error("Error analyzing raw audio data for word recognition.");
	};
	process16bitAudioStream(*audioStream.get(), processBuffer, progressSink);

	// End recognition
	error = ps_end_utt(&decoder);
	if (error) throw runtime_error("Error ending utterance processing for word recognition.");

	// Collect words
	BoundedTimeline<string> result(audioStream->getTruncatedRange());
	for (ps_seg_t* it = ps_seg_iter(&decoder); it; it = ps_seg_next(it)) {
		const char* word = ps_seg_word(it);
		int firstFrame, lastFrame;
		ps_seg_frames(it, &firstFrame, &lastFrame);
		result.set(centiseconds(firstFrame), centiseconds(lastFrame + 1), word);
	}

	return result;
}

s3wid_t getWordId(const string& word, dict_t& dictionary) {
	s3wid_t wordId = dict_wordid(&dictionary, word.c_str());
	if (wordId == BAD_S3WID) throw invalid_argument(fmt::format("Unknown word '{}'.", word));
	return wordId;
}

optional<BoundedTimeline<Phone>> getPhoneAlignment(
	const vector<s3wid_t>& wordIds,
	unique_ptr<AudioStream> audioStream,
	ps_decoder_t& decoder,
	ProgressSink& progressSink)
{
	// Create alignment list
	lambda_unique_ptr<ps_alignment_t> alignment(
		ps_alignment_init(decoder.d2p),
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
	acmod_t* acousticModel = decoder.acmod;
	lambda_unique_ptr<ps_search_t> search(
		state_align_search_init("state_align", decoder.config, acousticModel, alignment.get()),
		[](ps_search_t* search) { ps_search_free(search); });
	if (!search) throw runtime_error("Error creating search.");

	// Start recognition
	error = acmod_start_utt(acousticModel);
	if (error) throw runtime_error("Error starting utterance processing for alignment.");

	{
		// Eventually end recognition
		auto endRecognition = gsl::finally([&]() { acmod_end_utt(acousticModel); });

		// Start search
		ps_search_start(search.get());

		// Process entire sound stream
		auto processBuffer = [&](const vector<int16_t>& buffer) {
			const int16* nextSample = buffer.data();
			size_t remainingSamples = buffer.size();
			while (acmod_process_raw(acousticModel, &nextSample, &remainingSamples, false) > 0) {
				while (acousticModel->n_feat_frame > 0) {
					ps_search_step(search.get(), acousticModel->output_frame);
					acmod_advance(acousticModel);
				}
			}
		};
		process16bitAudioStream(*audioStream.get(), processBuffer, progressSink);

		// End search
		error = ps_search_finish(search.get());
		if (error) return boost::none;
	}

	// Extract phones with timestamps
	char** phoneNames = decoder.dict->mdef->ciname;
	BoundedTimeline<Phone> result(audioStream->getTruncatedRange());
	for (ps_alignment_iter_t* it = ps_alignment_phones(alignment.get()); it; it = ps_alignment_iter_next(it)) {
		// Get phone
		ps_alignment_entry_t* phoneEntry = ps_alignment_iter_get(it);
		s3cipid_t phoneId = phoneEntry->id.pid.cipid;
		string phoneName = phoneNames[phoneId];

		if (phoneName == "SIL") continue;

		// Add entry
		centiseconds start(phoneEntry->start);
		centiseconds duration(phoneEntry->duration);
		Timed<Phone> timedPhone(start, start + duration, PhoneConverter::get().parse(phoneName));
		result.set(timedPhone);
	}
	return result;
}

bool dictionaryContains(dict_t& dictionary, const string& word) {
	return dict_wordid(&dictionary, word.c_str()) != BAD_S3WID;
}

void addMissingDictionaryWords(const vector<string>& words, ps_decoder_t& decoder) {
	map<string, string> missingPronunciations;
	for (const string& word : words) {
		if (!dictionaryContains(*decoder.dict, word)) {
			string pronunciation;
			for (Phone phone : wordToPhones(word)) {
				if (pronunciation.length() > 0) pronunciation += " ";
				pronunciation += PhoneConverter::get().toString(phone);
			}
			missingPronunciations[word] = pronunciation;
		}
	}
	for (auto it = missingPronunciations.begin(); it != missingPronunciations.end(); ++it) {
		bool isLast = it == --missingPronunciations.end();
		logging::infoFormat("Unknown word '{}'. Guessing pronunciation '{}'.", it->first, it->second);
		ps_add_word(&decoder, it->first.c_str(), it->second.c_str(), isLast);
	}
}

lambda_unique_ptr<ps_decoder_t> createDecoder(optional<u32string> dialog) {
	lambda_unique_ptr<cmd_ln_t> config(
		cmd_ln_init(
			nullptr, ps_args(), true,
			// Set acoustic model
			"-hmm", (getSphinxModelDirectory() / "acoustic-model").string().c_str(),
			// Set pronunciation dictionary
			"-dict", (getSphinxModelDirectory() / "cmudict-en-us.dict").string().c_str(),
			// Add noise against zero silence (see http://cmusphinx.sourceforge.net/wiki/faq#qwhy_my_accuracy_is_poor)
			"-dither", "yes",
			nullptr),
		[](cmd_ln_t* config) { cmd_ln_free_r(config); });
	if (!config) throw runtime_error("Error creating configuration.");

	lambda_unique_ptr<ps_decoder_t> decoder(
		ps_init(config.get()),
		[](ps_decoder_t* recognizer) { ps_free(recognizer); });
	if (!decoder) throw runtime_error("Error creating speech decoder.");

	// Set language model
	lambda_unique_ptr<ngram_model_t> languageModel;
	if (dialog) {
		// Create dialog-specific language model
		vector<string> words = tokenizeText(*dialog, [&](const string& word) { return dictionaryContains(*decoder->dict, word); });
		words.insert(words.begin(), "<s>");
		words.push_back("</s>");
		languageModel = createLanguageModel(words, *decoder->lmath);

		// Add any dialog-specific words to the dictionary
		addMissingDictionaryWords(words, *decoder);
	} else {
		path modelPath = getSphinxModelDirectory() / "en-us.lm.bin";
		languageModel = lambda_unique_ptr<ngram_model_t>(
			ngram_model_read(decoder->config, modelPath.string().c_str(), NGRAM_AUTO, decoder->lmath),
			[](ngram_model_t* lm) { ngram_model_free(lm); });
	}
	ps_set_lm(decoder.get(), "lm", languageModel.get());
	ps_set_search(decoder.get(), "lm");

	return decoder;
}

BoundedTimeline<Phone> detectPhones(
	unique_ptr<AudioStream> audioStream,
	optional<u32string> dialog,
	ProgressSink& progressSink)
{
	// Discard Pocketsphinx output
	err_set_logfp(nullptr);

	// Redirect Pocketsphinx output to log
	err_set_callback(sphinxLogCallback, nullptr);

	// Make sure audio stream has no DC offset
	audioStream = removeDCOffset(std::move(audioStream));

	ProgressMerger totalProgressMerger(progressSink);
	ProgressSink& voiceActivationProgressSink = totalProgressMerger.addSink(1.0);
	ProgressSink& dialogProgressSink = totalProgressMerger.addSink(15);

	try {
		// Split audio into utterances
		BoundedTimeline<void> utterances = detectVoiceActivity(audioStream->clone(true), voiceActivationProgressSink);

		// For progress reporting: weigh utterances by length
		ProgressMerger dialogProgressMerger(dialogProgressSink);
		vector<ProgressSink*> utteranceProgressSinks;
		for (const auto& timedUtterance : utterances) {
			utteranceProgressSinks.push_back(&dialogProgressMerger.addSink(timedUtterance.getTimeRange().getLength().count()));
		}
		auto utteranceProgressSinkIt = utteranceProgressSinks.begin();

		BoundedTimeline<Phone> result(audioStream->getTruncatedRange());
		std::mutex resultMutex;
		logging::debug("Speech recognition -- start");
		ObjectPool<ps_decoder_t> decoderPool([&dialog] { return createDecoder(dialog).release(); });
		ThreadPool threadPool;
		for (const auto& timedUtterance : utterances) {
			threadPool.addJob([&] {
				auto decoder = decoderPool.acquire();
				ProgressMerger utteranceProgressMerger(**utteranceProgressSinkIt++);
				ProgressSink& wordRecognitionProgressSink = utteranceProgressMerger.addSink(1.0);
				ProgressSink& alignmentProgressSink = utteranceProgressMerger.addSink(0.5);

				const TimeRange timeRange = timedUtterance.getTimeRange();
				logging::logTimedEvent("utterance", timeRange, string(""));

				auto streamSegment = createSegment(audioStream->clone(true), timeRange);

				// Get words
				BoundedTimeline<string> words = recognizeWords(streamSegment->clone(true), *decoder.get(), wordRecognitionProgressSink);
				for (Timed<string> timedWord : words) {
					timedWord.getTimeRange().shift(timedUtterance.getStart());
					logging::logTimedEvent("word", timedWord);
				}
			
				// Look up words in dictionary
				vector<s3wid_t> wordIds;
				for (const auto& timedWord : words) {
					wordIds.push_back(getWordId(timedWord.getValue(), *decoder->dict));
				}
				if (wordIds.empty()) return;

				// Align the words' phones with speech
				BoundedTimeline<Phone> segmentPhones = getPhoneAlignment(wordIds, std::move(streamSegment), *decoder.get(), alignmentProgressSink)
					.value_or(ContinuousTimeline<Phone>(streamSegment->getTruncatedRange(), Phone::Unknown));
				segmentPhones.shift(timedUtterance.getStart());
				for (const auto& timedPhone : segmentPhones) {
					logging::logTimedEvent("phone", timedPhone);
				}

				// Fill result
				std::lock_guard<std::mutex> lock(resultMutex);
				for (const auto& timedPhone : segmentPhones) {
					result.set(timedPhone);
				}
			});
		}
		threadPool.waitAll();
		logging::debug("Speech recognition -- end");

		return result;
	}
	catch (...) {
		std::throw_with_nested(runtime_error("Error performing speech recognition via Pocketsphinx."));
	}
}
