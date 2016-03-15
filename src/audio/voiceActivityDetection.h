#pragma once
#include <vector>
#include <TimeSegment.h>
#include <memory>
#include "AudioStream.h"

std::vector<TimeSegment> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream);
