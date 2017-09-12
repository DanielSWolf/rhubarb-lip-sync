#pragma once

#include "audio/AudioClip.h"
#include "core/Phone.h"
#include "tools/ProgressBar.h"
#include "time/BoundedTimeline.h"

BoundedTimeline<Phone> recognizePhones(
	const AudioClip& audioClip,
	boost::optional<std::string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink);
