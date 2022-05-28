#pragma once

#include "Recognizer.h"
#include "pocketSphinxTools.h"

class PocketSphinxRecognizer : public Recognizer {
public:
	BoundedTimeline<Phone> recognizePhones(
		const AudioClip& inputAudioClip,
		boost::optional<std::string> dialog,
		boost::optional<BoundedTimeline<Phone>> alignedPhones,
		int maxThreadCount,
		ProgressSink& progressSink
	) const override;
};
