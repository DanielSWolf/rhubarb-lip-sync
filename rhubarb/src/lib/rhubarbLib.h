#pragma once

#include <filesystem>

#include "animation/targetShapeSet.h"
#include "audio/AudioClip.h"
#include "core/Shape.h"
#include "recognition/Recognizer.h"
#include "time/ContinuousTimeline.h"
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
