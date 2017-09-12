#pragma once

#include "core/Shape.h"
#include "time/ContinuousTimeline.h"
#include "audio/AudioClip.h"
#include "tools/ProgressBar.h"
#include <boost/filesystem.hpp>
#include "animation/targetShapeSet.h"

JoiningContinuousTimeline<Shape> animateAudioClip(
	const AudioClip& audioClip,
	boost::optional<std::string> dialog,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink);

JoiningContinuousTimeline<Shape> animateWaveFile(
	boost::filesystem::path filePath,
	boost::optional<std::string> dialog,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink);
