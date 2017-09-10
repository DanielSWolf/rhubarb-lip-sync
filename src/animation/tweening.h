#pragma once

#include "core/Shape.h"
#include "time/ContinuousTimeline.h"

// Takes an existing animation and inserts inbetween shapes for smoother results.
JoiningContinuousTimeline<Shape> insertTweens(const JoiningContinuousTimeline<Shape>& animation);
