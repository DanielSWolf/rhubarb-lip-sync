#pragma once

#include "core/shape.h"
#include "time/continuous-timeline.h"

// Takes an existing animation and modifies the pauses (X shapes) to look better.
JoiningContinuousTimeline<Shape> animatePauses(const JoiningContinuousTimeline<Shape>& animation);
