#pragma once

#include "Phone.h"
#include "animationRules.h"
#include "BoundedTimeline.h"
#include "ContinuousTimeline.h"

// A shape set with its original phone
using ShapeRule = std::tuple<ShapeSet, boost::optional<Phone>>;

// Returns shape rules for an entire timeline of phones.
ContinuousTimeline<ShapeRule> getShapeRules(const BoundedTimeline<Phone>& phones);
