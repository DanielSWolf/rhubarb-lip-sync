#pragma once
#include <memory>
#include "AudioStream.h"
#include <Timeline.h>

Timeline<bool> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream);
