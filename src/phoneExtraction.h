#pragma once

#include <map>
#include <memory>
#include <functional>
#include "audioInput/AudioStream.h"
#include "Phone.h"
#include "centiseconds.h"
#include "progressBar.h"

std::map<centiseconds, Phone> detectPhones(std::function<std::unique_ptr<AudioStream>(void)> createAudioStream, ProgressSink& progressSink);
