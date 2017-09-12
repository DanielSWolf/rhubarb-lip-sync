#pragma once

#include "core/Shape.h"
#include "time/ContinuousTimeline.h"
#include "ShapeRule.h"
#include <functional>

using AnimationFunction = std::function<JoiningContinuousTimeline<Shape>(const ContinuousTimeline<ShapeRule>&)>;

// Calls the specified animation function with the specified shape rules.
// If the resulting animation contains long static segments, the shape rules are tweaked and animated again.
// Static segments happen rather often.
// See http://animateducated.blogspot.de/2016/10/lip-sync-animation-2.html?showComment=1478861729702#c2940729096183546458.
JoiningContinuousTimeline<Shape> avoidStaticSegments(const ContinuousTimeline<ShapeRule>& shapeRules, AnimationFunction animate);
