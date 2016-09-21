#pragma once

#include "audio/AudioClip.h"
#include "Phone.h"
#include "ProgressBar.h"
#include "BoundedTimeline.h"

BoundedTimeline<Phone> recognizePhones(
	const AudioClip& audioClip,
	boost::optional<std::u32string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink);
