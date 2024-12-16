#pragma once

#include "core/phone.h"
#include "core/shape.h"
#include "target-shape-set.h"
#include "time/continuous-timeline.h"

JoiningContinuousTimeline<Shape> animate(
    const BoundedTimeline<Phone>& phones, const ShapeSet& targetShapeSet
);
