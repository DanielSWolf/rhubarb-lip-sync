#pragma once

#include <vector>
#include <functional>
#include "audio/AudioStream.h"
#include "ProgressBar.h"

void process16bitAudioStream(AudioStream& audioStream, std::function<void(const std::vector<int16_t>&)> processBuffer, size_t bufferCapacity, ProgressSink& progressSink);
void process16bitAudioStream(AudioStream& audioStream, std::function<void(const std::vector<int16_t>&)> processBuffer, ProgressSink& progressSink);