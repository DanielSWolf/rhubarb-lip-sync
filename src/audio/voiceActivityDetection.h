#pragma once
#include <vector>
#include <TimeRange.h>
#include <memory>
#include "AudioStream.h"

std::vector<TimeRange> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream);
