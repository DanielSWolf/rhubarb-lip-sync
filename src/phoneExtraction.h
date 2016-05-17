#pragma once

#include <memory>
#include "audio/AudioStream.h"
#include "Phone.h"
#include "progressBar.h"
#include "BoundedTimeline.h"

BoundedTimeline<Phone> detectPhones(
	std::unique_ptr<AudioStream> audioStream,
	ProgressSink& progressSink);
