#pragma once
#include "AudioClip.h"
#include "time/BoundedTimeline.h"
#include "tools/ProgressBar.h"

JoiningBoundedTimeline<void> detectVoiceActivity(const AudioClip& audioClip, int maxThreadCount, ProgressSink& progressSink);
