#pragma once

#include <memory>
#include "audio/AudioStream.h"
#include "Phone.h"
#include "progressBar.h"
#include <boost/optional/optional.hpp>
#include "BoundedTimeline.h"

BoundedTimeline<Phone> detectPhones(
	std::unique_ptr<AudioStream> audioStream,
	boost::optional<std::string> dialog,
	ProgressSink& progressSink);
