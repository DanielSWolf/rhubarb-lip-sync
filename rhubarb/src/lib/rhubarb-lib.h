#pragma once

#include <filesystem>

#include "animation/target-shape-set.h"
#include "audio/audio-clip.h"
#include "core/shape.h"
#include "recognition/recognizer.h"
#include "time/continuous-timeline.h"
#include "tools/progress.h"

JoiningContinuousTimeline<Shape> animateAudioClip(
    const AudioClip& audioClip,
    const boost::optional<std::string>& dialog,
    const Recognizer& recognizer,
    const ShapeSet& targetShapeSet,
    int maxThreadCount,
    ProgressSink& progressSink
);

JoiningContinuousTimeline<Shape> animateWaveFile(
    std::filesystem::path filePath,
    const boost::optional<std::string>& dialog,
    const Recognizer& recognizer,
    const ShapeSet& targetShapeSet,
    int maxThreadCount,
    ProgressSink& progressSink
);
