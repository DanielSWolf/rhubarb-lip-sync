#ifndef LIPSYNC_WAVEFILEREADER16KHZMONO_H
#define LIPSYNC_WAVEFILEREADER16KHZMONO_H

#include "AudioStream.h"
#include <memory>
#include <string>

std::unique_ptr<AudioStream> create16kHzMonoStream(std::string fileName);

#endif //LIPSYNC_WAVEFILEREADER16KHZMONO_H
