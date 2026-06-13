#include "exporterTools.h"
#include "animation/targetShapeSet.h"
#include <vector>

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(
	const JoiningTimeline<Shape>& animation,
	const ShapeSet& targetShapeSet
) {
	std::vector<Timed<Shape>> result;
	std::copy(animation.begin(), animation.end(), std::back_inserter(result));
	if (result.empty()) {
		// Add zero-length empty mouth
		result.emplace_back(0_cs, 0_cs, convertToTargetShapeSet(Shape::X, targetShapeSet));
	}
	return result;
}

JoiningContinuousTimeline<Shape> mergeShortCues(
	const JoiningContinuousTimeline<Shape>& animation,
	centiseconds minDuration
) {
	if (minDuration <= 0_cs) {
		return animation;
	}

	std::vector<Timed<Shape>> cues(animation.begin(), animation.end());

	if (cues.empty()) {
		return animation;
	}

	// Forward pass: absorb each short cue into the preceding one.
	// The first cue is handled separately below.
	for (std::size_t i = 1; i < cues.size(); ++i) {
		if (cues[i].getDuration() < minDuration) {
			cues[i] = Timed<Shape>(cues[i].getTimeRange(), cues[i - 1].getValue());
		}
	}

	// If the very first cue is short, pull the shape from the (possibly
	// already-updated) second cue so the timeline still starts cleanly.
	if (cues[0].getDuration() < minDuration && cues.size() > 1) {
		cues[0] = Timed<Shape>(cues[0].getTimeRange(), cues[1].getValue());
	}

	// Rebuild — JoiningContinuousTimeline auto-merges adjacent equal values.
	JoiningContinuousTimeline<Shape> result(
		animation.getRange(),
		Shape::X,
		cues.begin(),
		cues.end()
	);
	return result;
}
