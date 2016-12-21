#pragma once

#include "Shape.h"
#include "ContinuousTimeline.h"
#include "AudioClip.h"
#include "ProgressBar.h"
#include <boost/filesystem.hpp>
#include "targetShapeSet.h"

JoiningContinuousTimeline<Shape> animateAudioClip(
	const AudioClip& audioClip,
	boost::optional<std::u32string> dialog,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink);

JoiningContinuousTimeline<Shape> animateWaveFile(
	boost::filesystem::path filePath,
	boost::optional<std::u32string> dialog,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink);
