#pragma once

#include "core/Shape.h"
#include "time/Timeline.h"
#include "time/ContinuousTimeline.h"
#include "time/centiseconds.h"

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(
	const JoiningTimeline<Shape>& animation,
	const ShapeSet& targetShapeSet
);

// Merges cues shorter than minDuration into their neighbors.
// Short cues are replaced by the shape of the preceding cue (or the
// following cue for the very first segment). This prevents sub-frame
// flicker when the output is consumed at a fixed frame rate.
JoiningContinuousTimeline<Shape> mergeShortCues(
	const JoiningContinuousTimeline<Shape>& animation,
	centiseconds minDuration
);
