#pragma once

#include "ShapeRule.h"

// Does a rough animation (no tweening, special pause animation, etc.) using a bidirectional algorithm.
JoiningContinuousTimeline<Shape> animateRough(const ContinuousTimeline<ShapeRule>& shapeRules);
