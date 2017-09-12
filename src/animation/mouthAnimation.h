#pragma once

#include "core/Phone.h"
#include "core/Shape.h"
#include "time/ContinuousTimeline.h"
#include "targetShapeSet.h"

JoiningContinuousTimeline<Shape> animate(const BoundedTimeline<Phone>& phones, const ShapeSet& targetShapeSet);
