#pragma once

#include "audio/AudioClip.h"
#include "core/Phone.h"
#include "tools/progress.h"
#include "time/BoundedTimeline.h"

class Recognizer {
public:
	virtual ~Recognizer() = default;

	virtual BoundedTimeline<Phone> recognizePhones(
		const AudioClip& audioClip,
		boost::optional<std::string> dialog,
		boost::optional<BoundedTimeline<Phone>> alignedPhones,
		int maxThreadCount,
		ProgressSink& progressSink
	) const = 0;
};