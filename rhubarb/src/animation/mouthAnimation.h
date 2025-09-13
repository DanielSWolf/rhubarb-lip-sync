#pragma once

#include "core/Phone.h"
#include "core/Shape.h"
#include "targetShapeSet.h"
#include "time/ContinuousTimeline.h"

JoiningContinuousTimeline<Shape> animate(
    const BoundedTimeline<Phone>& phones, const ShapeSet& targetShapeSet
);
