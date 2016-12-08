#pragma once

#include "Shape.h"
#include "Timeline.h"

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(const JoiningTimeline<Shape>& shapes);
