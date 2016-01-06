#pragma once

#include <map>
#include <chrono>
#include <ratio>
#include <memory>
#include "audioInput/AudioStream.h"
#include "Phone.h"
#include "centiseconds.h"

std::map<centiseconds, Phone> detectPhones(std::unique_ptr<AudioStream> audioStream);
