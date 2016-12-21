#pragma once

#include "Phone.h"
#include "Shape.h"
#include "ContinuousTimeline.h"
#include "targetShapeSet.h"

JoiningContinuousTimeline<Shape> animate(const BoundedTimeline<Phone>& phones, const ShapeSet& targetShapeSet);
