#pragma once

#include "core/Shape.h"
#include "time/ContinuousTimeline.h"
#include "audio/AudioClip.h"
#include "tools/progress.h"
#include <filesystem>
#include "animation/targetShapeSet.h"
#include "recognition/Recognizer.h"

JoiningContinuousTimeline<Shape> animateAudioClip(
	const AudioClip& audioClip,
	const boost::optional<std::string>& dialog,
	const Recognizer& recognizer,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink);

JoiningContinuousTimeline<Shape> animateWaveFile(
	std::filesystem::path filePath,
	const boost::optional<std::string>& dialog,
	const Recognizer& recognizer,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink);
