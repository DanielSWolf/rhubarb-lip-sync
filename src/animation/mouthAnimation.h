#pragma once

#include "Phone.h"
#include "Shape.h"
#include "ContinuousTimeline.h"

JoiningContinuousTimeline<Shape> animate(const BoundedTimeline<Phone>& phones);
