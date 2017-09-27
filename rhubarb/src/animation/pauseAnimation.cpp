#include "pauseAnimation.h"
#include "animationRules.h"

Shape getPauseShape(Shape previous, Shape next, centiseconds duration) {
	// For very short pauses: Just hold the previous shape
	if (duration < 12_cs) {
		return previous;
	}

	// For short pauses: Relax the mouth
	if (duration <= 35_cs) {
		// It looks odd if the pause shape is identical to the next shape.
		// Make sure we find a relaxed shape that's different from the next one.
		for (Shape currentRelaxedShape = previous;;) {
			Shape nextRelaxedShape = relax(currentRelaxedShape);
			if (nextRelaxedShape != next) {
				return nextRelaxedShape;
			}
			if (nextRelaxedShape == currentRelaxedShape) {
				// We're going in circles
				break;
			}
			currentRelaxedShape = nextRelaxedShape;
		}
	}

	// For longer pauses: Close the mouth
	return Shape::X;
}

JoiningContinuousTimeline<Shape> animatePauses(const JoiningContinuousTimeline<Shape>& animation) {
	JoiningContinuousTimeline<Shape> result(animation);
	
	for_each_adjacent(animation.begin(), animation.end(), [&](const Timed<Shape>& previous, const Timed<Shape>& pause, const Timed<Shape>& next) {
		if (pause.getValue() != Shape::X) return;

		result.set(pause.getTimeRange(), getPauseShape(previous.getValue(), next.getValue(), pause.getDuration()));
	});

	return result;
}
