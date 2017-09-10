#pragma once

#include "core/Shape.h"
#include "time/ContinuousTimeline.h"

// Takes an existing animation and modifies the pauses (X shapes) to look better.
JoiningContinuousTimeline<Shape> animatePauses(const JoiningContinuousTimeline<Shape>& animation);
