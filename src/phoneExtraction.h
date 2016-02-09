#pragma once

#include <map>
#include <memory>
#include <functional>
#include "audioInput/AudioStream.h"
#include "Phone.h"
#include "centiseconds.h"
#include "progressBar.h"
#include <boost/optional/optional.hpp>

std::map<centiseconds, Phone> detectPhones(
	std::function<std::unique_ptr<AudioStream>(void)> createAudioStream,
	boost::optional<std::string> dialog,
	ProgressSink& progressSink);
