#pragma once

#include "Shape.h"
#include "ContinuousTimeline.h"

// Changes the timing of an existing animation to reduce jitter and to make sure all shapes register visually.
// In some cases, shapes may be omitted.
JoiningContinuousTimeline<Shape> optimizeTiming(const JoiningContinuousTimeline<Shape>& animation);
