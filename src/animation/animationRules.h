#pragma once

#include <set>
#include "Shape.h"
#include "Timeline.h"
#include "Phone.h"

// Returns the basic shape (A-F) that most closely resembles the specified shape.
Shape getBasicShape(Shape shape);

// Returns the mouth shape that results from relaxing the specified shape.
Shape relax(Shape shape);

// Returns the mouth shape to use for *short* pauses between words.
Shape getRelaxedBridge(Shape lhs, Shape rhs);

// A set of mouth shapes that can be used to represent a certain sound
using ShapeSet = std::set<Shape>;

// Gets the shape from a non-empty set of shapes that most closely resembles a reference shape.
Shape getClosestShape(Shape reference, ShapeSet shapes);

// Indicates how to time a tween between two mouth shapes
enum class TweenTiming {
	// Tween should end at the original transition
	Early,

	// Tween should overlap both original mouth shapes equally
	Centered,

	// Tween should begin at the original transition
	Late
};

// Returns the tween shape and timing to use to transition between the specified two mouth shapes.
boost::optional<std::pair<Shape, TweenTiming>> getTween(Shape first, Shape second);

// A struct describing the possible shapes to use during a given time range.
struct ShapeRule {
	ShapeRule(const ShapeSet& regularShapes, const ShapeSet& alternativeShapes = {});

	// A set of one or more shapes that may be used to animate a given time range.
	// The actual selection will be performed based on similarity with the previous or next shape.
	ShapeSet regularShapes;

	// The regular animation algorithm tries to minimize mouth shape changes. As a result, the mouth may sometimes remain static for too long.
	// This is a set of zero or more shapes that may be used in these cases.
	// In contrast to the regular shapes, this set should only contain shapes that can be used regardless of the surrounding shapes.
	ShapeSet alternativeShapes;

	bool operator==(const ShapeRule& rhs) const {
		return regularShapes == rhs.regularShapes && alternativeShapes == rhs.alternativeShapes;
	}

	bool operator!=(const ShapeRule& rhs) const {
		return !operator==(rhs);
	}
};

// Returns the shape rule(s) to use for a given phone.
// The resulting timeline will always cover the entire duration of the phone (starting at 0 cs).
// It may extend into the negative time range if animation is required prior to the sound being heard.
Timeline<ShapeRule> getShapeRules(Phone phone, centiseconds duration, centiseconds previousDuration);

