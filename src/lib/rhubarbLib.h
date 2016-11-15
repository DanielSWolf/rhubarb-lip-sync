#pragma once

#include "Shape.h"
#include "ContinuousTimeline.h"
#include "AudioClip.h"
#include "ProgressBar.h"
#include <boost/filesystem.hpp>

ContinuousTimeline<Shape> animateAudioClip(
	const AudioClip& audioClip,
	boost::optional<std::u32string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink);

ContinuousTimeline<Shape> animateWaveFile(
	boost::filesystem::path filePath,
	boost::optional<std::u32string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink);
