#ifndef LIPSYNC_PHONE_EXTRACTION_H
#define LIPSYNC_PHONE_EXTRACTION_H

#include <map>
#include <chrono>
#include <ratio>
#include <memory>
#include "audioInput/AudioStream.h"
#include "Phone.h"
#include "centiseconds.h"

std::map<centiseconds, Phone> detectPhones(std::unique_ptr<AudioStream> audioStream);

#endif //LIPSYNC_PHONE_EXTRACTION_H
