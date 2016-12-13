#include "mouthAnimation.h"
#include "timedLogging.h"
#include "shapeRule.h"
#include "roughAnimation.h"
#include "pauseAnimation.h"
#include "tweening.h"
#include "timingOptimization.h"

JoiningContinuousTimeline<Shape> animate(const BoundedTimeline<Phone> &phones) {
	// Create timeline of shape rules
	const ContinuousTimeline<ShapeRule> shapeRules = getShapeRules(phones);

	// Animate in multiple steps
	JoiningContinuousTimeline<Shape> animation = animateRough(shapeRules);
	animation = optimizeTiming(animation);
	animation = animatePauses(animation);
	animation = insertTweens(animation);

	for (const auto& timedShape : animation) {
		logTimedEvent("shape", timedShape);
	}

	return animation;
}
