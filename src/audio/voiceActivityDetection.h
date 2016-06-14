#pragma once
#include <memory>
#include "AudioStream.h"
#include <BoundedTimeline.h>
#include <ProgressBar.h>

BoundedTimeline<void> detectVoiceActivity(std::unique_ptr<AudioStream> audioStream, ProgressSink& progressSink);
