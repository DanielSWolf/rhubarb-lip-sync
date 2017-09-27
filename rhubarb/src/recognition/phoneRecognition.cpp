#include <boost/filesystem.hpp>
#include "phoneRecognition.h"
#include "audio/SampleRateConverter.h"
#include "tools/platformTools.h"
#include "tools/tools.h"
#include <format.h>
#include <s3types.h>
#include <regex>
#include <gsl_util.h>
#include "logging/logging.h"
#include "audio/DcOffset.h"
#include "time/Timeline.h"
#include "audio/voiceActivityDetection.h"
#include "audio/AudioSegment.h"
#include "languageModels.h"
#include "tokenization.h"
#include "g2p.h"
#include "time/ContinuousTimeline.h"
#include "audio/processing.h"
#include "tools/parallel.h"
#include <boost/version.hpp>
#include "tools/ObjectPool.h"
#include "time/timedLogging.h"

extern "C" {
#include <pocketsphinx.h>
#include <sphinxbase/err.h>
#include <ps_alignment.h>
#include <state_align_search.h>
#include <pocketsphinx_internal.h>
#include <ngram_search.h>
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
using std::string;
using std::chrono::duration_cast;
using std::array;

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

BoundedTimeline<string> recognizeWords(const vector<int16_t>& audioBuffer, ps_decoder_t& decoder) {
	// Restart timing at 0
	ps_start_stream(&decoder);

	// Start recognition
	int error = ps_start_utt(&decoder);
	if (error) throw runtime_error("Error starting utterance processing for word recognition.");

	// Process entire audio clip
	const bool noRecognition = false;
	const bool fullUtterance = true;
	int searchedFrameCount = ps_process_raw(&decoder, audioBuffer.data(), audioBuffer.size(), noRecognition, fullUtterance);
	if (searchedFrameCount < 0) throw runtime_error("Error analyzing raw audio data for word recognition.");

	// End recognition
	error = ps_end_utt(&decoder);
	if (error) throw runtime_error("Error ending utterance processing for word recognition.");

	BoundedTimeline<string> result(TimeRange(0_cs, centiseconds(100 * audioBuffer.size() / sphinxSampleRate)));
	bool noWordsRecognized = reinterpret_cast<ngram_search_t*>(decoder.search)->bpidx == 0;
	if (noWordsRecognized) {
		return result;
	}

	// Collect words
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

optional<Timeline<Phone>> getPhoneAlignment(
	const vector<s3wid_t>& wordIds,
	const vector<int16_t>& audioBuffer,
	ps_decoder_t& decoder)
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

		// Process entire audio clip
		const int16* nextSample = audioBuffer.data();
		size_t remainingSamples = audioBuffer.size();
		bool fullUtterance = true;
		while (acmod_process_raw(acousticModel, &nextSample, &remainingSamples, fullUtterance) > 0) {
			while (acousticModel->n_feat_frame > 0) {
				ps_search_step(search.get(), acousticModel->output_frame);
				acmod_advance(acousticModel);
			}
		}

		// End search
		error = ps_search_finish(search.get());
		if (error) return boost::none;
	}

	// Extract phones with timestamps
	char** phoneNames = decoder.dict->mdef->ciname;
	Timeline<Phone> result;
	for (ps_alignment_iter_t* it = ps_alignment_phones(alignment.get()); it; it = ps_alignment_iter_next(it)) {
		// Get phone
		ps_alignment_entry_t* phoneEntry = ps_alignment_iter_get(it);
		s3cipid_t phoneId = phoneEntry->id.pid.cipid;
		string phoneName = phoneNames[phoneId];

		if (phoneName == "SIL") continue;

		// Add entry
		centiseconds start(phoneEntry->start);
		centiseconds duration(phoneEntry->duration);
		Phone phone = PhoneConverter::get().parse(phoneName);
		if (phone == Phone::AH && duration < 6_cs) {
			// Heuristic: < 6_cs is schwa. Pocketsphinx doesn't differentiate.
			phone = Phone::Schwa;
		}
		Timed<Phone> timedPhone(start, start + duration, phone);
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

lambda_unique_ptr<ngram_model_t> createDefaultLanguageModel(ps_decoder_t& decoder) {
	path modelPath = getSphinxModelDirectory() / "en-us.lm.bin";
	lambda_unique_ptr<ngram_model_t> result(
		ngram_model_read(decoder.config, modelPath.string().c_str(), NGRAM_AUTO, decoder.lmath),
		[](ngram_model_t* lm) { ngram_model_free(lm); });
	if (!result) {
		throw runtime_error(fmt::format("Error reading language model from {}.", modelPath));
	}

	return std::move(result);
}

lambda_unique_ptr<ngram_model_t> createDialogLanguageModel(ps_decoder_t& decoder, const string& dialog) {
	// Split dialog into normalized words
	vector<string> words = tokenizeText(dialog, [&](const string& word) { return dictionaryContains(*decoder.dict, word); });

	// Add dialog-specific words to the dictionary
	addMissingDictionaryWords(words, decoder);

	// Create dialog-specific language model
	words.insert(words.begin(), "<s>");
	words.push_back("</s>");
	return createLanguageModel(words, decoder);
}

lambda_unique_ptr<ngram_model_t> createBiasedLanguageModel(ps_decoder_t& decoder, const string& dialog) {
	auto defaultLanguageModel = createDefaultLanguageModel(decoder);
	auto dialogLanguageModel = createDialogLanguageModel(decoder, dialog);
	constexpr int modelCount = 2;
	array<ngram_model_t*, modelCount> languageModels{ defaultLanguageModel.get(), dialogLanguageModel.get() };
	array<char*, modelCount> modelNames{ "defaultLM", "dialogLM" };
	array<float, modelCount> modelWeights{ 0.1f, 0.9f };
	lambda_unique_ptr<ngram_model_t> result(
		ngram_model_set_init(nullptr, languageModels.data(), modelNames.data(), modelWeights.data(), modelCount),
		[](ngram_model_t* lm) { ngram_model_free(lm); });
	if (!result) {
		throw runtime_error("Error creating biased language model.");
	}

	return std::move(result);
}

lambda_unique_ptr<ps_decoder_t> createDecoder(optional<string> dialog) {
	lambda_unique_ptr<cmd_ln_t> config(
		cmd_ln_init(
			nullptr, ps_args(), true,
			// Set acoustic model
			"-hmm", (getSphinxModelDirectory() / "acoustic-model").string().c_str(),
			// Set pronunciation dictionary
			"-dict", (getSphinxModelDirectory() / "cmudict-en-us.dict").string().c_str(),
			// Add noise against zero silence (see http://cmusphinx.sourceforge.net/wiki/faq#qwhy_my_accuracy_is_poor)
			"-dither", "yes",
			// Disable VAD -- we're doing that ourselves
			"-remove_silence", "no",
			// Perform per-utterance cepstral mean normalization
			"-cmn", "batch",
			nullptr),
		[](cmd_ln_t* config) { cmd_ln_free_r(config); });
	if (!config) throw runtime_error("Error creating configuration.");

	lambda_unique_ptr<ps_decoder_t> decoder(
		ps_init(config.get()),
		[](ps_decoder_t* recognizer) { ps_free(recognizer); });
	if (!decoder) throw runtime_error("Error creating speech decoder.");

	// Set language model
	lambda_unique_ptr<ngram_model_t> languageModel(dialog
		? createBiasedLanguageModel(*decoder, *dialog)
		: createDefaultLanguageModel(*decoder));
	ps_set_lm(decoder.get(), "lm", languageModel.get());
	ps_set_search(decoder.get(), "lm");

	return decoder;
}

JoiningTimeline<void> getNoiseSounds(TimeRange utteranceTimeRange, const Timeline<Phone>& phones) {
	JoiningTimeline<void> noiseSounds;

	// Find utterance parts without recogniced phones
	noiseSounds.set(utteranceTimeRange);
	for (const auto& timedPhone : phones) {
		noiseSounds.clear(timedPhone.getTimeRange());
	}

	// Remove undesired elements
	const centiseconds minSoundDuration = 12_cs;
	for (const auto& unknownSound : JoiningTimeline<void>(noiseSounds)) {
		bool startsAtZero = unknownSound.getStart() == 0_cs;
		bool tooShort = unknownSound.getDuration() < minSoundDuration;
		if (startsAtZero || tooShort) {
			noiseSounds.clear(unknownSound.getTimeRange());
		}
	}

	return noiseSounds;
}

// Some words have multiple pronunciations, one of which results in better animation than the others.
// This function returns the optimal pronunciation for a select set of these words.
string fixPronunciation(const string& word) {
	const static map<string, string> replacements {
		{"into(2)", "into"},
		{"to(2)", "to"},
		{"to(3)", "to"},
		{"today(2)", "today"},
		{"tomorrow(2)", "tomorrow"},
		{"tonight(2)", "tonight"}
	};

	const auto pair = replacements.find(word);
	return pair != replacements.end() ? pair->second : word;
}

Timeline<Phone> utteranceToPhones(
	const AudioClip& audioClip,
	TimeRange utteranceTimeRange,
	ps_decoder_t& decoder,
	ProgressSink& utteranceProgressSink)
{
	ProgressMerger utteranceProgressMerger(utteranceProgressSink);
	ProgressSink& wordRecognitionProgressSink = utteranceProgressMerger.addSink(1.0);
	ProgressSink& alignmentProgressSink = utteranceProgressMerger.addSink(0.5);

	// Pad time range to give Pocketsphinx some breathing room
	TimeRange paddedTimeRange = utteranceTimeRange;
	const centiseconds padding(3);
	paddedTimeRange.grow(padding);
	paddedTimeRange.trim(audioClip.getTruncatedRange());

	const unique_ptr<AudioClip> clipSegment = audioClip.clone() | segment(paddedTimeRange) | resample(sphinxSampleRate);
	const auto audioBuffer = copyTo16bitBuffer(*clipSegment);

	// Get words
	BoundedTimeline<string> words = recognizeWords(audioBuffer, decoder);
	wordRecognitionProgressSink.reportProgress(1.0);

	// Log utterance text
	string text;
	for (auto& timedWord : words) {
		string word = timedWord.getValue();
		// Skip details
		if (word == "<s>" || word == "</s>" || word == "<sil>") {
			continue;
		}
		word = regex_replace(word, regex("\\(\\d\\)"), "");
		if (text.size() > 0) {
			text += " ";
		}
		text += word;
	}
	logTimedEvent("utterance", utteranceTimeRange, text);

	// Log words
	for (Timed<string> timedWord : words) {
		timedWord.getTimeRange().shift(paddedTimeRange.getStart());
		logTimedEvent("word", timedWord);
	}

	// Convert word strings to word IDs using dictionary
	vector<s3wid_t> wordIds;
	for (const auto& timedWord : words) {
		const string fixedWord = fixPronunciation(timedWord.getValue());
		wordIds.push_back(getWordId(fixedWord, *decoder.dict));
	}
	if (wordIds.empty()) return {};

	// Align the words' phones with speech
#if BOOST_VERSION < 105600 // Support legacy syntax
#define value_or get_value_or
#endif
	Timeline<Phone> utterancePhones = getPhoneAlignment(wordIds, audioBuffer, decoder)
		.value_or(ContinuousTimeline<Phone>(clipSegment->getTruncatedRange(), Phone::Noise));
	alignmentProgressSink.reportProgress(1.0);
	utterancePhones.shift(paddedTimeRange.getStart());

	// Log raw phones
	for (const auto& timedPhone : utterancePhones) {
		logTimedEvent("rawPhone", timedPhone);
	}

	// Guess positions of noise sounds
	JoiningTimeline<void> noiseSounds = getNoiseSounds(utteranceTimeRange, utterancePhones);
	for (const auto& noiseSound : noiseSounds) {
		utterancePhones.set(noiseSound.getTimeRange(), Phone::Noise);
	}

	// Log phones
	for (const auto& timedPhone : utterancePhones) {
		logTimedEvent("phone", timedPhone);
	}

	return utterancePhones;
}

BoundedTimeline<Phone> recognizePhones(
	const AudioClip& inputAudioClip,
	optional<string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink)
{
	ProgressMerger totalProgressMerger(progressSink);
	ProgressSink& voiceActivationProgressSink = totalProgressMerger.addSink(1.0);
	ProgressSink& dialogProgressSink = totalProgressMerger.addSink(15);

	// Make sure audio stream has no DC offset
	const unique_ptr<AudioClip> audioClip = inputAudioClip.clone() | removeDcOffset();

	// Split audio into utterances
	JoiningBoundedTimeline<void> utterances;
	try {
		utterances = detectVoiceActivity(*audioClip, maxThreadCount, voiceActivationProgressSink);
	}
	catch (...) {
		std::throw_with_nested(runtime_error("Error detecting segments of speech."));
	}

	// Discard Pocketsphinx output
	err_set_logfp(nullptr);

	// Redirect Pocketsphinx output to log
	err_set_callback(sphinxLogCallback, nullptr);

	// Prepare pool of decoders
	ObjectPool<ps_decoder_t, lambda_unique_ptr<ps_decoder_t>> decoderPool(
		[&dialog] { return createDecoder(dialog); });

	BoundedTimeline<Phone> phones(audioClip->getTruncatedRange());
	std::mutex resultMutex;
	auto processUtterance = [&](Timed<void> timedUtterance, ProgressSink& utteranceProgressSink) {
		// Detect phones for utterance
		auto decoder = decoderPool.acquire();
		Timeline<Phone> utterancePhones =
			utteranceToPhones(*audioClip, timedUtterance.getTimeRange(), *decoder, utteranceProgressSink);

		// Copy phones to result timeline
		std::lock_guard<std::mutex> lock(resultMutex);
		for (const auto& timedPhone : utterancePhones) {
			phones.set(timedPhone);
		}
	};

	auto getUtteranceProgressWeight = [](const Timed<void> timedUtterance) {
		return timedUtterance.getDuration().count();
	};

	// Perform speech recognition
	try {
		// Determine how many parallel threads to use
		int threadCount = std::min({
			maxThreadCount,
			// Don't use more threads than there are utterances to be processed
			static_cast<int>(utterances.size()),
			// Don't waste time creating additional threads (and decoders!) if the recording is short
			static_cast<int>(duration_cast<std::chrono::seconds>(audioClip->getTruncatedRange().getDuration()).count() / 5)
		});
		if (threadCount < 1) {
			threadCount = 1;
		}
		logging::debugFormat("Speech recognition using {} threads -- start", threadCount);
		runParallel(processUtterance, utterances, threadCount, dialogProgressSink, getUtteranceProgressWeight);
		logging::debug("Speech recognition -- end");
	}
	catch (...) {
		std::throw_with_nested(runtime_error("Error performing speech recognition via Pocketsphinx."));
	}

	return phones;
}
