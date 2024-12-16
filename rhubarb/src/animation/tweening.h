#pragma once

#include "core/shape.h"
#include "time/continuous-timeline.h"

// Takes an existing animation and inserts inbetween shapes for smoother results.
JoiningContinuousTimeline<Shape> insertTweens(const JoiningContinuousTimeline<Shape>& animation);
