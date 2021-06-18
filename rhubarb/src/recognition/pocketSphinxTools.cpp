#include "pocketSphinxTools.h"

#include "tools/platformTools.h"
#include <regex>
#include "audio/DcOffset.h"
#include "audio/voiceActivityDetection.h"
#include "tools/parallel.h"
#include "tools/ObjectPool.h"
#include "time/timedLogging.h"

extern "C" {
#include <sphinxbase/err.h>
#include <pocketsphinx_internal.h>
#include <ngram_search.h>
}

using std::runtime_error;
using std::invalid_argument;
using std::unique_ptr;
using std::string;
using std::vector;
using std::filesystem::path;
using std::regex;
using boost::optional;
using std::chrono::duration_cast;
	
logging::Level convertSphinxErrorLevel(err_lvl_t errorLevel) {
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
		const int charsWritten = vsnprintf(chars.data(), chars.size(), format, args);
		if (charsWritten < 0) throw runtime_error("Error formatting PocketSphinx log message.");

		success = charsWritten < static_cast<int>(chars.size());
		if (!success) chars.resize(chars.size() * 2);
	}
	const regex waste("^(DEBUG|INFO|INFOCONT|WARN|ERROR|FATAL): ");
	string message =
		std::regex_replace(chars.data(), waste, "", std::regex_constants::format_first_only);
	boost::algorithm::trim(message);

	const logging::Level logLevel = convertSphinxErrorLevel(errorLevel);
	logging::log(logLevel, message);
}

void redirectPocketSphinxOutput() {
	static bool redirected = false;
	if (redirected) return;

	// Discard PocketSphinx output
	err_set_logfp(nullptr);

	// Redirect PocketSphinx output to log
	err_set_callback(sphinxLogCallback, nullptr);

	redirected = true;
}

BoundedTimeline<Phone> recognizePhones(
	const AudioClip& inputAudioClip,
	optional<std::string> dialog,
	decoderFactory createDecoder,
	utteranceToPhonesFunction utteranceToPhones,
	int maxThreadCount,
	ProgressSink& progressSink
) {
	ProgressMerger totalProgressMerger(progressSink);
	ProgressSink& voiceActivationProgressSink =
		totalProgressMerger.addSource("VAD (PocketSphinx tools)", 1.0);
	ProgressSink& dialogProgressSink =
		totalProgressMerger.addSource("recognition (PocketSphinx tools)", 15.0);

	// Make sure audio stream has no DC offset
	const unique_ptr<AudioClip> audioClip = inputAudioClip.clone() | removeDcOffset();

	// Split audio into utterances
	JoiningBoundedTimeline<void> utterances;
	try {
		utterances = detectVoiceActivity(*audioClip, voiceActivationProgressSink);
	} catch (...) {
		std::throw_with_nested(runtime_error("Error detecting segments of speech."));
	}

	redirectPocketSphinxOutput();

	// Prepare pool of decoders
	ObjectPool<ps_decoder_t, lambda_unique_ptr<ps_decoder_t>> decoderPool(
		[&] { return createDecoder(dialog); });

	BoundedTimeline<Phone> phones(audioClip->getTruncatedRange());
	std::mutex resultMutex;
	const auto processUtterance = [&](Timed<void> timedUtterance, ProgressSink& utteranceProgressSink) {
		// Detect phones for utterance
		const auto decoder = decoderPool.acquire();
		Timeline<Phone> utterancePhones = utteranceToPhones(
			*audioClip,
			timedUtterance.getTimeRange(),
			*decoder,
			utteranceProgressSink
		);

		// Copy phones to result timeline
		std::lock_guard<std::mutex> lock(resultMutex);
		for (const auto& timedPhone : utterancePhones) {
			phones.set(timedPhone);
		}
	};

	const auto getUtteranceProgressWeight = [](const Timed<void> timedUtterance) {
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
			static_cast<int>(
				duration_cast<std::chrono::seconds>(audioClip->getTruncatedRange().getDuration()).count() / 5
			)
		});
		if (threadCount < 1) {
			threadCount = 1;
		}
		logging::debugFormat("Speech recognition using {} threads -- start", threadCount);
		runParallel(
			"speech recognition (PocketSphinx tools)",
			processUtterance,
			utterances,
			threadCount,
			dialogProgressSink,
			getUtteranceProgressWeight
		);
		logging::debug("Speech recognition -- end");
	} catch (...) {
		std::throw_with_nested(runtime_error("Error performing speech recognition via PocketSphinx tools."));
	}

	return phones;
}

const path& getSphinxModelDirectory() {
	static path sphinxModelDirectory(getBinDirectory() / "res" / "sphinx");
	return sphinxModelDirectory;
}

JoiningTimeline<void> getNoiseSounds(TimeRange utteranceTimeRange, const Timeline<Phone>& phones) {
	JoiningTimeline<void> noiseSounds;

	// Find utterance parts without recognized phones
	noiseSounds.set(utteranceTimeRange);
	for (const auto& timedPhone : phones) {
		noiseSounds.clear(timedPhone.getTimeRange());
	}

	// Remove undesired elements
	const centiseconds minSoundDuration = 12_cs;
	for (const auto& unknownSound : JoiningTimeline<void>(noiseSounds)) {
		const bool startsAtZero = unknownSound.getStart() == 0_cs;
		const bool tooShort = unknownSound.getDuration() < minSoundDuration;
		if (startsAtZero || tooShort) {
			noiseSounds.clear(unknownSound.getTimeRange());
		}
	}

	return noiseSounds;
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
	const int searchedFrameCount =
		ps_process_raw(&decoder, audioBuffer.data(), audioBuffer.size(), noRecognition, fullUtterance);
	if (searchedFrameCount < 0) {
		throw runtime_error("Error analyzing raw audio data for word recognition.");
	}

	// End recognition
	error = ps_end_utt(&decoder);
	if (error) throw runtime_error("Error ending utterance processing for word recognition.");

	BoundedTimeline<string> result(
		TimeRange(0_cs, centiseconds(100 * audioBuffer.size() / sphinxSampleRate))
	);
	const bool phonetic = cmd_ln_boolean_r(decoder.config, "-allphone_ci");
	if (!phonetic) {
		// If the decoder is in word mode (as opposed to phonetic recognition), it expects each
		// utterance to contain speech. If it doesn't, ps_seg_word() logs the annoying error
		// "Couldn't find <s> in first frame".
		// Not every utterance does contain speech, however. In this case, we exit early to prevent
		// the log output.
		// We *don't* to that in phonetic mode because here, the same code would omit valid phones.
		const bool noWordsRecognized = reinterpret_cast<ngram_search_t*>(decoder.search)->bpidx == 0;
		if (noWordsRecognized) {
			return result;
		}
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
