#include "mouthAnimation.h"
#include "logging.h"
#include <unordered_set>
#include <unordered_map>
#include <boost/algorithm/clamp.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "timedLogging.h"
#include "shapeShorthands.h"
#include "animationRules.h"

using std::map;
using std::unordered_set;
using std::unordered_map;
using std::vector;
using boost::optional;
using boost::make_optional;
using std::chrono::duration_cast;
using boost::algorithm::clamp;
using boost::adaptors::transformed;
using std::pair;
using std::tuple;

Timeline<Shape> createTweens(ContinuousTimeline<Shape> shapes) {
	centiseconds minTweenDuration = 4_cs;
	centiseconds maxTweenDuration = 10_cs;

	Timeline<Shape> tweens;

	for (auto first = shapes.begin(), second = std::next(shapes.begin());
		first != shapes.end() && second != shapes.end();
		++first, ++second)
	{
		auto pair = getTween(first->getValue(), second->getValue());
		if (!pair) continue;

		Shape tweenShape;
		TweenTiming tweenTiming;
		std::tie(tweenShape, tweenTiming) = *pair;
		TimeRange firstTimeRange = first->getTimeRange();
		TimeRange secondTimeRange = second->getTimeRange();

		centiseconds tweenStart, tweenDuration;
		switch (tweenTiming) {
		case TweenTiming::Early: {
				tweenDuration = std::min(firstTimeRange.getDuration() / 3, maxTweenDuration);
				tweenStart = firstTimeRange.getEnd() - tweenDuration;
				break;
			}
		case TweenTiming::Centered: {
				tweenDuration = std::min({ firstTimeRange.getDuration() / 3, secondTimeRange.getDuration() / 3, maxTweenDuration });
				tweenStart = firstTimeRange.getEnd() - tweenDuration / 2;
				break;
			}
		case TweenTiming::Late: {
				tweenDuration = std::min(secondTimeRange.getDuration() / 3, maxTweenDuration);
				tweenStart = secondTimeRange.getStart();
				break;
			}
		}

		if (tweenDuration < minTweenDuration) continue;

		tweens.set(tweenStart, tweenStart + tweenDuration, tweenShape);
	}

	return tweens;
}

Timeline<Shape> animatePauses(const ContinuousTimeline<Shape>& shapes) {
	Timeline<Shape> result;

	// Don't close mouth for short pauses
	for_each_adjacent(shapes.begin(), shapes.end(), [&](const Timed<Shape>& lhs, const Timed<Shape>& pause, const Timed<Shape>& rhs) {
		if (pause.getValue() != X) return;

		const centiseconds maxPausedOpenMouthDuration = 35_cs;
		const TimeRange timeRange = pause.getTimeRange();
		if (timeRange.getDuration() <= maxPausedOpenMouthDuration) {
			result.set(timeRange, getRelaxedBridge(lhs.getValue(), rhs.getValue()));
		}
	});

	// Keep mouth open into pause if it just opened
	for_each_adjacent(shapes.begin(), shapes.end(), [&](const Timed<Shape>& secondLast, const Timed<Shape>& last, const Timed<Shape>& pause) {
		if (pause.getValue() != X) return;

		centiseconds lastDuration = last.getDuration();
		const centiseconds minOpenDuration = 20_cs;
		if (isClosed(secondLast.getValue()) && !isClosed(last.getValue()) && lastDuration < minOpenDuration) {
			const centiseconds minSpillDuration = 20_cs;
			centiseconds spillDuration = std::min(minSpillDuration, pause.getDuration());
			result.set(pause.getStart(), pause.getStart() + spillDuration, getRelaxedBridge(last.getValue(), X));
		}
	});

	return result;
}

template<typename T>
ContinuousTimeline<optional<T>> boundedTimelinetoContinuousOptional(const BoundedTimeline<T>& timeline) {
	return {
		timeline.getRange(), boost::none,
		timeline | transformed([](const Timed<T>& timedValue) { return Timed<optional<T>>(timedValue.getTimeRange(), timedValue.getValue()); })
	};
}

ContinuousTimeline<ShapeRule> getShapeRules(const BoundedTimeline<Phone>& phones) {
	// Convert to continuous timeline so that silences aren't skipped when iterating
	auto continuousPhones = boundedTimelinetoContinuousOptional(phones);

	// Create timeline of shape rules
	ContinuousTimeline<ShapeRule> shapeRules(phones.getRange(), {{X}});
	centiseconds previousDuration = 0_cs;
	for (const auto& timedPhone : continuousPhones) {
		optional<Phone> phone = timedPhone.getValue();
		centiseconds duration = timedPhone.getDuration();

		if (phone) {
			// Animate one phone
			Timeline<ShapeRule> phoneShapeRules = getShapeRules(*phone, duration, previousDuration);

			// Result timing is relative to phone. Make absolute.
			phoneShapeRules.shift(timedPhone.getStart());

			// Copy to timeline.
			// Later shape rules may overwrite earlier ones if overlapping.
			for (const auto& timedShapeRule : phoneShapeRules) {
				shapeRules.set(timedShapeRule);
			}
		}

		previousDuration = duration;
	}

	return shapeRules;
}

// Create timeline of shape rules using a bidirectional algorithm.
// Here's a rough sketch:
//
// * Most consonants result in shape sets with multiple options; most vowels have only one shape option.
// * When speaking, we tend to slur mouth shapes into each other. So we animate from start to end,
//   always choosing a shape from the current set that resembles the last shape and is somewhat relaxed.
// * When speaking, we anticipate vowels, trying to form their shape before the actual vowel.
//   So whenever we come across a one-shape set, we backtrack a little, spreating that shape to the left.
ContinuousTimeline<Shape> animate(const ContinuousTimeline<ShapeSet>& shapeSets) {
	ContinuousTimeline<Shape> shapes(shapeSets.getRange(), X);

	Shape referenceShape = X;
	// Animate forwards
	centiseconds lastAnticipatedShapeStart = -1_cs;
	for (auto it = shapeSets.begin(); it != shapeSets.end(); ++it) {
		const ShapeSet shapeSet = it->getValue();
		const Shape shape = getClosestShape(referenceShape, shapeSet);
		shapes.set(it->getTimeRange(), shape);
		const bool anticipateShape = shapeSet.size() == 1 && *shapeSet.begin() != X;
		if (anticipateShape) {
			// Animate backwards a little
			const Shape anticipatedShape = shape;
			const centiseconds anticipatedShapeStart = it->getStart();
			referenceShape = anticipatedShape;
			for (auto reverseIt = it; reverseIt != shapeSets.begin(); ) {
				--reverseIt;

				// Make sure we haven't animated too far back
				centiseconds anticipatingShapeStart = reverseIt->getStart();
				if (anticipatingShapeStart == lastAnticipatedShapeStart) break;
				const centiseconds maxAnticipationDuration = 20_cs;
				const centiseconds anticipationDuration = anticipatedShapeStart - anticipatingShapeStart;
				if (anticipationDuration > maxAnticipationDuration) break;

				// Make sure the new, backwards-animated shape still resembles the anticipated shape
				const Shape anticipatingShape = getClosestShape(referenceShape, reverseIt->getValue());
				if (getBasicShape(anticipatingShape) != getBasicShape(anticipatedShape)) break;

				// Overwrite forward-animated shape with backwards-animated, anticipating shape
				shapes.set(reverseIt->getTimeRange(), anticipatingShape);

				referenceShape = anticipatingShape;
			}
			lastAnticipatedShapeStart = anticipatedShapeStart;
		}
		referenceShape = anticipateShape ? shape : relax(shape);
	}

	return shapes;
}

ContinuousTimeline<Shape> animate(const BoundedTimeline<Phone> &phones) {
	// Create timeline of shape rules
	ContinuousTimeline<ShapeRule> shapeRules = getShapeRules(phones);

	// Take only the regular shapes from each shape rule. Alternative shapes will be implemented later.
	ContinuousTimeline<ShapeSet> shapeSets(
		shapeRules.getRange(), {{X}},
		shapeRules | transformed([](const Timed<ShapeRule>& timedRule) { return Timed<ShapeSet>(timedRule.getTimeRange(), timedRule.getValue().regularShapes); }));

	// Animate
	ContinuousTimeline<Shape> shapes = animate(shapeSets);

	// Animate pauses
	Timeline<Shape> pauses = animatePauses(shapes);
	for (const auto& pause : pauses) {
		shapes.set(pause);
	}

	// Create inbetweens for smoother animation
	Timeline<Shape> tweens = createTweens(shapes);
	for (const auto& tween : tweens) {
		shapes.set(tween);
	}

	for (const auto& timedShape : shapes) {
		logTimedEvent("shape", timedShape);
	}

	return shapes;
}
