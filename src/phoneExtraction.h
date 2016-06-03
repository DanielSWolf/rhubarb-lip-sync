#pragma once

#include <memory>
#include "audio/AudioStream.h"
#include "Phone.h"
#include "progressBar.h"
#include "BoundedTimeline.h"

BoundedTimeline<Phone> detectPhones(
	std::unique_ptr<AudioStream> audioStream,
	boost::optional<std::u32string> dialog,
	ProgressSink& progressSink);
