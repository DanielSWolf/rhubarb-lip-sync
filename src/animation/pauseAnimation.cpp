#include "pauseAnimation.h"
#include "animationRules.h"

JoiningContinuousTimeline<Shape> animatePauses(const JoiningContinuousTimeline<Shape>& shapes) {
	JoiningContinuousTimeline<Shape> result(shapes);

	// Don't close mouth for short pauses
	for_each_adjacent(shapes.begin(), shapes.end(), [&](const Timed<Shape>& lhs, const Timed<Shape>& pause, const Timed<Shape>& rhs) {
		if (pause.getValue() != Shape::X) return;

		const centiseconds maxPausedOpenMouthDuration = 35_cs;
		const TimeRange timeRange = pause.getTimeRange();
		if (timeRange.getDuration() <= maxPausedOpenMouthDuration) {
			result.set(timeRange, getRelaxedBridge(lhs.getValue(), rhs.getValue()));
		}
	});

	// Keep mouth open into pause if it just opened
	for_each_adjacent(shapes.begin(), shapes.end(), [&](const Timed<Shape>& secondLast, const Timed<Shape>& last, const Timed<Shape>& pause) {
		if (pause.getValue() != Shape::X) return;

		centiseconds lastDuration = last.getDuration();
		const centiseconds minOpenDuration = 20_cs;
		if (isClosed(secondLast.getValue()) && !isClosed(last.getValue()) && lastDuration < minOpenDuration) {
			const centiseconds minSpillDuration = 20_cs;
			centiseconds spillDuration = std::min(minSpillDuration, pause.getDuration());
			result.set(pause.getStart(), pause.getStart() + spillDuration, getRelaxedBridge(last.getValue(), Shape::X));
		}
	});

	return result;
}
