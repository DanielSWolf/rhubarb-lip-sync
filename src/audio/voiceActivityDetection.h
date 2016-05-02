#pragma once
#include <memory>
#include "AudioStream.h"
#include <BoundedTimeline.h>

BoundedTimeline<void> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream);
