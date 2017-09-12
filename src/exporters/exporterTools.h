#pragma once

#include "core/Shape.h"
#include "time/Timeline.h"

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(const JoiningTimeline<Shape>& animation, const ShapeSet& targetShapeSet);
