#pragma once

#include <map>
#include <memory>
#include "audioInput/AudioStream.h"
#include "Phone.h"
#include "centiseconds.h"

std::map<centiseconds, Phone> detectPhones(std::unique_ptr<AudioStream> audioStream, std::function<void(double)> reportProgress);
