#ifndef LIPSYNC_WAVEFILEWRITER_H
#define LIPSYNC_WAVEFILEWRITER_H

#include <memory>
#include <string>
#include "AudioStream.h"

void createWaveFile(std::unique_ptr<AudioStream> inputStream, std::string fileName);

#endif //LIPSYNC_WAVEFILEWRITER_H
