#pragma once

#include <map>
#include <memory>
#include "audio/AudioStream.h"
#include "Phone.h"
#include "centiseconds.h"
#include "progressBar.h"
#include <boost/optional/optional.hpp>

std::map<centiseconds, Phone> detectPhones(
	std::unique_ptr<AudioStream> audioStream,
	boost::optional<std::string> dialog,
	ProgressSink& progressSink);
