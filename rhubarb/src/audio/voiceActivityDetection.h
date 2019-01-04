#pragma once
#include "AudioClip.h"
#include "time/BoundedTimeline.h"
#include "tools/progress.h"

JoiningBoundedTimeline<void> detectVoiceActivity(
	const AudioClip& audioClip,
	ProgressSink& progressSink
);
