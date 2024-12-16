#pragma once
#include "audio-clip.h"
#include "time/bounded-timeline.h"
#include "tools/progress.h"

JoiningBoundedTimeline<void> detectVoiceActivity(
    const AudioClip& audioClip, ProgressSink& progressSink
);
