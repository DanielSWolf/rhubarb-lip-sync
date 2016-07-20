#pragma once
#include "AudioClip.h"
#include <BoundedTimeline.h>
#include <ProgressBar.h>

BoundedTimeline<void> detectVoiceActivity(const AudioClip& audioClip, ProgressSink& progressSink);
