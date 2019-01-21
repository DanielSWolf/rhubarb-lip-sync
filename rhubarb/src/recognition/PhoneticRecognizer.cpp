#include "PhoneticRecognizer.h"
#include "time/Timeline.h"
#include "audio/AudioSegment.h"
#include "audio/SampleRateConverter.h"
#include "audio/processing.h"
#include "time/timedLogging.h"

using std::runtime_error;
using std::unique_ptr;
using std::string;
using boost::optional;

static lambda_unique_ptr<ps_decoder_t> createDecoder(optional<std::string> dialog) {
	UNUSED(dialog);

	lambda_unique_ptr<cmd_ln_t> config(
		cmd_ln_init(
			nullptr, ps_args(), true,
			// Set acoustic model
			"-hmm", (getSphinxModelDirectory() / "acoustic-model").string().c_str(),
			// Set phonetic language model
			"-allphone", (getSphinxModelDirectory() / "en-us-phone.lm.bin").string().c_str(),
			"-allphone_ci", "yes",
			// Set language model probability weight.
			// Low values (<= 0.4) can lead to fluttering animation.
			// High values (>= 1.0) can lead to imprecise or freezing animation.
			"-lw", "0.8",
			// Add noise against zero silence
			// (see http://cmusphinx.sourceforge.net/wiki/faq#qwhy_my_accuracy_is_poor)
			"-dither", "yes",
			// Disable VAD -- we're doing that ourselves
			"-remove_silence", "no",
			// Perform per-utterance cepstral mean normalization
			"-cmn", "batch",

			// The following settings are recommended at
			// http://cmusphinx.sourceforge.net/wiki/phonemerecognition

			// Set beam width applied to every frame in Viterbi search
			"-beam", "1e-20",
			// Set beam width applied to phone transitions
			"-pbeam", "1e-20",
			nullptr),
		[](cmd_ln_t* config) { cmd_ln_free_r(config); });
	if (!config) throw runtime_error("Error creating configuration.");

	lambda_unique_ptr<ps_decoder_t> decoder(
		ps_init(config.get()),
		[](ps_decoder_t* recognizer) { ps_free(recognizer); });
	if (!decoder) throw runtime_error("Error creating speech decoder.");

	return decoder;
}

static Timeline<Phone> utteranceToPhones(
	const AudioClip& audioClip,
	TimeRange utteranceTimeRange,
	ps_decoder_t& decoder,
	ProgressSink& utteranceProgressSink
) {
	// Pad time range to give PocketSphinx some breathing room
	TimeRange paddedTimeRange = utteranceTimeRange;
	const centiseconds padding(3);
	paddedTimeRange.grow(padding);
	paddedTimeRange.trim(audioClip.getTruncatedRange());

	const unique_ptr<AudioClip> clipSegment = audioClip.clone()
		| segment(paddedTimeRange)
		| resample(sphinxSampleRate);
	const auto audioBuffer = copyTo16bitBuffer(*clipSegment);

	// Detect phones (returned as words)
	BoundedTimeline<string> phoneStrings = recognizeWords(audioBuffer, decoder);
	phoneStrings.shift(paddedTimeRange.getStart());
	Timeline<Phone> utterancePhones;
	for (const auto& timedPhoneString : phoneStrings) {
		Phone phone = PhoneConverter::get().parse(timedPhoneString.getValue());
		if (phone == Phone::AH && timedPhoneString.getDuration() < 6_cs) {
			// Heuristic: < 6_cs is schwa. PocketSphinx doesn't differentiate.
			phone = Phone::Schwa;
		}
		utterancePhones.set(timedPhoneString.getTimeRange(), phone);
	}

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

	utteranceProgressSink.reportProgress(1.0);

	return utterancePhones;
}

BoundedTimeline<Phone> PhoneticRecognizer::recognizePhones(
	const AudioClip& inputAudioClip,
	optional<std::string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink
) const {
	return ::recognizePhones(inputAudioClip, dialog, &createDecoder, &utteranceToPhones, maxThreadCount, progressSink);
}
