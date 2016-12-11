#pragma once

#include "Shape.h"
#include "ContinuousTimeline.h"

// Takes an existing animation and inserts inbetween shapes for smoother results.
JoiningContinuousTimeline<Shape> insertTweens(const JoiningContinuousTimeline<Shape>& shapes);
