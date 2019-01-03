#pragma once

#include <vector>
#include <functional>
#include "AudioClip.h"
#include "tools/progress.h"

void process16bitAudioClip(
	const AudioClip& audioClip,
	const std::function<void(const std::vector<int16_t>&)>& processBuffer,
	size_t bufferCapacity,
	ProgressSink& progressSink
);

void process16bitAudioClip(
	const AudioClip& audioClip,
	const std::function<void(const std::vector<int16_t>&)>& processBuffer,
	ProgressSink& progressSink
);

std::vector<int16_t> copyTo16bitBuffer(const AudioClip& audioClip);