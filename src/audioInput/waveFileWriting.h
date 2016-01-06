#pragma once

#include <memory>
#include <string>
#include "AudioStream.h"

void createWaveFile(std::unique_ptr<AudioStream> inputStream, std::string fileName);
