#pragma once

#include <set>
#include <boost/optional.hpp>
#include "Shape.h"
#include "Timeline.h"
#include "Phone.h"

// A set of mouth shapes that can be used to represent a certain sound
using ShapeSet = std::set<Shape>;

struct ShapeRule {
	ShapeRule(const ShapeSet& regularShapes, const ShapeSet& alternativeShapes = {});

	ShapeSet regularShapes;
	ShapeSet alternativeShapes;
};

Timeline<ShapeRule> animatePhone(boost::optional<Phone> phone, centiseconds duration, centiseconds previousDuration);
