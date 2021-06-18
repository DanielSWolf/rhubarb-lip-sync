#pragma once

#include "time/BoundedTimeline.h"
#include "core/Phone.h"
#include "audio/AudioClip.h"
#include "tools/progress.h"
#include <filesystem>

extern "C" {
#include <pocketsphinx.h>
}

typedef std::function<lambda_unique_ptr<ps_decoder_t>(
	boost::optional<std::string> dialog
)> decoderFactory;

typedef std::function<Timeline<Phone>(
	const AudioClip& audioClip,
	TimeRange utteranceTimeRange,
	ps_decoder_t& decoder,
	ProgressSink& utteranceProgressSink
)> utteranceToPhonesFunction;

BoundedTimeline<Phone> recognizePhones(
	const AudioClip& inputAudioClip,
	boost::optional<std::string> dialog,
	decoderFactory createDecoder,
	utteranceToPhonesFunction utteranceToPhones,
	int maxThreadCount,
	ProgressSink& progressSink
);

constexpr int sphinxSampleRate = 16000;

const std::filesystem::path& getSphinxModelDirectory();

JoiningTimeline<void> getNoiseSounds(TimeRange utteranceTimeRange, const Timeline<Phone>& phones);

BoundedTimeline<std::string> recognizeWords(
	const std::vector<int16_t>& audioBuffer,
	ps_decoder_t& decoder
);
