#include "timingOptimization.h"
#include "time/timedLogging.h"
#include <boost/lexical_cast.hpp>
#include <map>
#include <algorithm>
#include "ShapeRule.h"

using std::string;
using std::map;

string getShapesString(const JoiningContinuousTimeline<Shape>& shapes) {
	string result;
	for (const auto& timedShape : shapes) {
		if (result.size()) {
			result.append(" ");
		}
		result.append(boost::lexical_cast<std::string>(timedShape.getValue()));
	}
	return result;
}

Shape getRepresentativeShape(const JoiningTimeline<Shape>& timeline) {
	if (timeline.empty()) {
		throw std::invalid_argument("Cannot determine representative shape from empty timeline.");
	}

	// Collect candidate shapes with weights
	map<Shape, centiseconds> candidateShapeWeights;
	for (const auto& timedShape : timeline) {
		candidateShapeWeights[timedShape.getValue()] += timedShape.getDuration();
	}

	// Select shape with highest total duration within the candidate range
	const Shape bestShape = std::max_element(
		candidateShapeWeights.begin(), candidateShapeWeights.end(),
		[](auto a, auto b) { return a.second < b.second; }
	)->first;

	// Shapes C and D are similar, but D is more interesting.
	const bool substituteD = bestShape == Shape::C && candidateShapeWeights[Shape::D] > 0_cs;
	return substituteD ? Shape::D : bestShape;
}

struct ShapeReduction {
	ShapeReduction(const JoiningTimeline<Shape>& sourceShapes) :
		sourceShapes(sourceShapes),
		shape(getRepresentativeShape(sourceShapes))
	{}

	ShapeReduction(const JoiningTimeline<Shape>& sourceShapes, TimeRange candidateRange) :
		ShapeReduction(JoiningBoundedTimeline<Shape>(candidateRange, sourceShapes))
	{}

	JoiningTimeline<Shape> sourceShapes;
	Shape shape;
};

// Returns a time range of candidate shapes for the next shape to draw.
// Guaranteed to be non-empty.
TimeRange getNextMinimalCandidateRange(const JoiningContinuousTimeline<Shape>& sourceShapes, const TimeRange targetRange, const centiseconds writePosition) {
	if (sourceShapes.empty()) {
		throw std::invalid_argument("Cannot determine candidate range for empty source timeline.");
	}

	// Too short, and and we get flickering. Too long, and too many shapes are lost.
	// Good values turn out to be 5 to 7 cs, with 7 cs sometimes looking just marginally better.
	const centiseconds minShapeDuration = 7_cs;

	// If the remaining time can hold more than one shape, but not two: split it evenly
	const centiseconds remainingTargetDuration = writePosition - targetRange.getStart();
	const bool canFitOneOrLess = remainingTargetDuration <= minShapeDuration;
	const bool canFitTwo = remainingTargetDuration >= 2 * minShapeDuration;
	const centiseconds duration = canFitOneOrLess || canFitTwo ? minShapeDuration : remainingTargetDuration / 2;

	TimeRange candidateRange(writePosition - duration, writePosition);
	if (writePosition == targetRange.getEnd()) {
		// This is the first iteration.
		// Extend the candidate range to the right in order to consider all source shapes after the target range.
		candidateRange.setEndIfLater(sourceShapes.getRange().getEnd());
	}
	if (candidateRange.getStart() >= sourceShapes.getRange().getEnd()) {
		// We haven't reached the source range yet.
		// Extend the candidate range to the left in order to encompass the right-most source shape.
		candidateRange.setStart(sourceShapes.rbegin()->getStart());
	}
	if (candidateRange.getEnd() <= sourceShapes.getRange().getStart()) {
		// We're past the source range. This can happen in corner cases.
		// Extend the candidate range to the right in order to encompass the left-most source shape
		candidateRange.setEnd(sourceShapes.begin()->getEnd());
	}

	return candidateRange;
}

ShapeReduction getNextShapeReduction(const JoiningContinuousTimeline<Shape>& sourceShapes, const TimeRange targetRange, centiseconds writePosition) {
	// Determine the next time range of candidate shapes. Consider two scenarios:

	// ... the shortest-possible candidate range
	const ShapeReduction minReduction(sourceShapes, getNextMinimalCandidateRange(sourceShapes, targetRange, writePosition));

	// ... a candidate range extended to the left to fully encompass its left-most shape
	const ShapeReduction extendedReduction(sourceShapes,
		{minReduction.sourceShapes.begin()->getStart(), minReduction.sourceShapes.getRange().getEnd()});

	// Determine the shape that might be picked *next* if we choose the shortest-possible candidate range now
	const ShapeReduction nextReduction(sourceShapes,
		getNextMinimalCandidateRange(sourceShapes, targetRange, minReduction.sourceShapes.getRange().getStart()));

	const bool minEqualsExtended = minReduction.shape == extendedReduction.shape;
	const bool extendedIsSpecial = extendedReduction.shape != minReduction.shape
		&& extendedReduction.shape != nextReduction.shape;

	return minEqualsExtended || extendedIsSpecial ? extendedReduction : minReduction;
}

// Modifies the timing of the given animation to fit into the specified target time range without jitter.
JoiningContinuousTimeline<Shape> retime(const JoiningContinuousTimeline<Shape>& sourceShapes, const TimeRange targetRange) {
	logTimedEvent("segment", targetRange, getShapesString(sourceShapes));

	JoiningContinuousTimeline<Shape> result(targetRange, Shape::X);
	if (sourceShapes.empty()) return result;

	// Animate backwards
	centiseconds writePosition = targetRange.getEnd();
	while (writePosition > targetRange.getStart()) {

		// Decide which shape to show next, possibly discarding short shapes
		const ShapeReduction shapeReduction = getNextShapeReduction(sourceShapes, targetRange, writePosition);

		// Determine how long to display the shape
		TimeRange targetShapeRange(shapeReduction.sourceShapes.getRange());
		if (targetShapeRange.getStart() <= sourceShapes.getRange().getStart()) {
			// We've used up the left-most source shape. Fill the entire remaining target range.
			targetShapeRange.setStartIfEarlier(targetRange.getStart());
		}
		targetShapeRange.trimRight(writePosition);

		// Draw shape
		result.set(targetShapeRange, shapeReduction.shape);

		writePosition = targetShapeRange.getStart();
	}

	return result;
}

JoiningContinuousTimeline<Shape> retime(const JoiningContinuousTimeline<Shape>& animation, TimeRange sourceRange, TimeRange targetRange) {
	const auto sourceShapes = JoiningContinuousTimeline<Shape>(sourceRange, Shape::X, animation);
	return retime(sourceShapes, targetRange);
}

enum class MouthState {
	Idle,
	Closed,
	Open
};

JoiningContinuousTimeline<Shape> optimizeTiming(const JoiningContinuousTimeline<Shape>& animation) {
	// Identify segments with idle, closed, and open mouth shapes
	JoiningContinuousTimeline<MouthState> segments(animation.getRange(), MouthState::Idle);
	for (const auto& timedShape : animation) {
		const Shape shape = timedShape.getValue();
		const MouthState mouthState = shape == Shape::X ? MouthState::Idle : shape == Shape::A ? MouthState::Closed : MouthState::Open;
		segments.set(timedShape.getTimeRange(), mouthState);
	}

	// The minimum duration a segment of open or closed mouth shapes must have to visually register
	const centiseconds minSegmentDuration = 8_cs;
	// The maximum amount by which the start of a shape can be brought forward
	const centiseconds maxExtensionDuration = 6_cs;

	// Make sure all open and closed segments are long enough to register visually.
	JoiningContinuousTimeline<Shape> result(animation.getRange(), Shape::X);
	// ... we're filling the result timeline from right to left, so `resultStart` points to the earliest shape already written
	centiseconds resultStart = result.getRange().getEnd();
	for (auto segmentIt = segments.rbegin(); segmentIt != segments.rend(); ++segmentIt) {
		// We don't care about idle shapes at this point.
		if (segmentIt->getValue() == MouthState::Idle) continue;

		resultStart = std::min(segmentIt->getEnd(), resultStart);
		if (resultStart - segmentIt->getStart() >= minSegmentDuration) {
			// The segment is long enough; we don't have to extend it to the left.
			const TimeRange targetRange(segmentIt->getStart(), resultStart);
			const auto retimedSegment = retime(animation, segmentIt->getTimeRange(), targetRange);
			for (const auto& timedShape : retimedSegment) {
				result.set(timedShape);
			}
			resultStart = targetRange.getStart();
		} else {
			// The segment is too short; we have to extend it to the left.
			// Find all adjacent segments to our left that are also too short, then distribute them evenly.
			const auto begin = segmentIt;
			auto end = std::next(begin);
			while (end != segments.rend() && end->getValue() != MouthState::Idle && end->getDuration() < minSegmentDuration) ++end;

			// Determine how much we should extend the entire set of short segments to the left
			const size_t shortSegmentCount = std::distance(begin, end);
			const centiseconds desiredDuration = minSegmentDuration * shortSegmentCount;
			const centiseconds currentDuration = begin->getEnd() - std::prev(end)->getStart();
			const centiseconds desiredExtensionDuration = desiredDuration - currentDuration;
			const centiseconds availableExtensionDuration = end != segments.rend() ? end->getDuration() - 1_cs : 0_cs;
			const centiseconds extensionDuration = std::min({desiredExtensionDuration, availableExtensionDuration, maxExtensionDuration});

			// Distribute available time range evenly among all short segments
			const centiseconds shortSegmentsTargetStart = std::prev(end)->getStart() - extensionDuration;
			for (auto shortSegmentIt = begin; shortSegmentIt != end; ++shortSegmentIt) {
				size_t remainingShortSegmentCount = std::distance(shortSegmentIt, end);
				const centiseconds segmentDuration = (resultStart - shortSegmentsTargetStart) / remainingShortSegmentCount;
				const TimeRange segmentTargetRange(resultStart - segmentDuration, resultStart);
				const auto retimedSegment = retime(animation, shortSegmentIt->getTimeRange(), segmentTargetRange);
				for (const auto& timedShape : retimedSegment) {
					result.set(timedShape);
				}
				resultStart = segmentTargetRange.getStart();
			}

			segmentIt = std::prev(end);
		}
	}

	return result;
}
