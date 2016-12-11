#include "mouthAnimation.h"
#include "timedLogging.h"
#include "shapeRule.h"
#include "roughAnimation.h"
#include "pauseAnimation.h"
#include "tweening.h"

JoiningContinuousTimeline<Shape> animate(const BoundedTimeline<Phone> &phones) {
	// Create timeline of shape rules
	const ContinuousTimeline<ShapeRule> shapeRules = getShapeRules(phones);

	// Animate
	JoiningContinuousTimeline<Shape> shapes = animateRough(shapeRules);
	shapes = animatePauses(shapes);
	shapes = insertTweens(shapes);

	for (const auto& timedShape : shapes) {
		logTimedEvent("shape", timedShape);
	}

	return shapes;
}
