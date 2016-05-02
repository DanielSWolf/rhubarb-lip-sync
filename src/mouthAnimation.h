#pragma once

#include "Phone.h"
#include "Shape.h"
#include "ContinuousTimeline.h"

ContinuousTimeline<Shape> animate(const BoundedTimeline<Phone>& phones);
