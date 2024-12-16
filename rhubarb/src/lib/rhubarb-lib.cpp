#include "rhubarb-lib.h"

#include "animation/mouth-animation.h"
#include "audio/audio-file-reading.h"
#include "core/phone.h"
#include "tools/text-files.h"

using boost::optional;
using std::string;
using std::filesystem::path;

JoiningContinuousTimeline<Shape> animateAudioClip(
    const AudioClip& audioClip,
    const optional<string>& dialog,
    const Recognizer& recognizer,
    const ShapeSet& targetShapeSet,
    int maxThreadCount,
    ProgressSink& progressSink
) {
    const BoundedTimeline<Phone> phones =
        recognizer.recognizePhones(audioClip, dialog, maxThreadCount, progressSink);
    JoiningContinuousTimeline<Shape> result = animate(phones, targetShapeSet);
    return result;
}

JoiningContinuousTimeline<Shape> animateWaveFile(
    path filePath,
    const optional<string>& dialog,
    const Recognizer& recognizer,
    const ShapeSet& targetShapeSet,
    int maxThreadCount,
    ProgressSink& progressSink
) {
    const auto audioClip = createAudioFileClip(filePath);
    return animateAudioClip(
        *audioClip, dialog, recognizer, targetShapeSet, maxThreadCount, progressSink
    );
}
