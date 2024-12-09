#pragma once

#include "audio/AudioClip.h"
#include "core/Phone.h"
#include "time/BoundedTimeline.h"
#include "tools/progress.h"

class Recognizer {
public:
    virtual ~Recognizer() = default;

    virtual BoundedTimeline<Phone> recognizePhones(
        const AudioClip& audioClip,
        boost::optional<std::string> dialog,
        int maxThreadCount,
        ProgressSink& progressSink
    ) const = 0;
};
