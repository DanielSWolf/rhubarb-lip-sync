#pragma once
#include "AudioClip.h"
#include <BoundedTimeline.h>
#include <ProgressBar.h>

JoiningBoundedTimeline<void> detectVoiceActivity(const AudioClip& audioClip, int maxThreadCount, ProgressSink& progressSink);
