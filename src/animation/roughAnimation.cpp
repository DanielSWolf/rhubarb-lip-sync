#include "roughAnimation.h"
#include <boost/optional.hpp>

using boost::optional;

// Create timeline of shapes using a bidirectional algorithm.
// Here's a rough sketch:
//
// * Most consonants result in shape sets with multiple options; most vowels have only one shape option.
// * When speaking, we tend to slur mouth shapes into each other. So we animate from start to end,
//   always choosing a shape from the current set that resembles the last shape and is somewhat relaxed.
// * When speaking, we anticipate vowels, trying to form their shape before the actual vowel.
//   So whenever we come across a one-shape vowel, we backtrack a little, spreating that shape to the left.
JoiningContinuousTimeline<Shape> animateRough(const ContinuousTimeline<ShapeRule>& shapeRules) {
	JoiningContinuousTimeline<Shape> animation(shapeRules.getRange(), Shape::X);

	Shape referenceShape = Shape::X;
	// Animate forwards
	centiseconds lastAnticipatedShapeStart = -1_cs;
	for (auto it = shapeRules.begin(); it != shapeRules.end(); ++it) {
		const ShapeRule shapeRule = it->getValue();
		const Shape shape = getClosestShape(referenceShape, shapeRule.shapeSet);
		animation.set(it->getTimeRange(), shape);
		const bool anticipateShape = shapeRule.phone && isVowel(*shapeRule.phone) && shapeRule.shapeSet.size() == 1;
		if (anticipateShape) {
			// Animate backwards a little
			const Shape anticipatedShape = shape;
			const centiseconds anticipatedShapeStart = it->getStart();
			referenceShape = anticipatedShape;
			for (auto reverseIt = it; reverseIt != shapeRules.begin(); ) {
				--reverseIt;

				// Make sure we haven't animated too far back
				centiseconds anticipatingShapeStart = reverseIt->getStart();
				if (anticipatingShapeStart == lastAnticipatedShapeStart) break;
				const centiseconds maxAnticipationDuration = 20_cs;
				const centiseconds anticipationDuration = anticipatedShapeStart - anticipatingShapeStart;
				if (anticipationDuration > maxAnticipationDuration) break;

				// Overwrite forward-animated shape with backwards-animated, anticipating shape
				const Shape anticipatingShape = getClosestShape(referenceShape, reverseIt->getValue().shapeSet);
				animation.set(reverseIt->getTimeRange(), anticipatingShape);

				// Make sure the new, backwards-animated shape still resembles the anticipated shape
				if (getBasicShape(anticipatingShape) != getBasicShape(anticipatedShape)) break;

				referenceShape = anticipatingShape;
			}
			lastAnticipatedShapeStart = anticipatedShapeStart;
		}
		referenceShape = anticipateShape ? shape : relax(shape);
	}

	return animation;
}
