#pragma once

#include "core/shape.h"
#include "time/timeline.h"

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(
    const JoiningTimeline<Shape>& animation, const ShapeSet& targetShapeSet
);
