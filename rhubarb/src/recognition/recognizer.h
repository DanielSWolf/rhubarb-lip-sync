#pragma once

#include "audio/audio-clip.h"
#include "core/phone.h"
#include "time/bounded-timeline.h"
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
