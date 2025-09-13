#pragma once

#include "pocketSphinxTools.h"
#include "Recognizer.h"

class PocketSphinxRecognizer : public Recognizer {
public:
    BoundedTimeline<Phone> recognizePhones(
        const AudioClip& inputAudioClip,
        boost::optional<std::string> dialog,
        int maxThreadCount,
        ProgressSink& progressSink
    ) const override;
};
