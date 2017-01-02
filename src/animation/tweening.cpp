#include "tweening.h"
#include "animationRules.h"

JoiningContinuousTimeline<Shape> insertTweens(const JoiningContinuousTimeline<Shape>& animation) {
	const centiseconds minTweenDuration = 4_cs;
	const centiseconds maxTweenDuration = 8_cs;

	JoiningContinuousTimeline<Shape> result(animation);

	for_each_adjacent(animation.begin(), animation.end(), [&](const auto& first, const auto& second) {
		auto pair = getTween(first.getValue(), second.getValue());
		if (!pair) return;

		Shape tweenShape;
		TweenTiming tweenTiming;
		std::tie(tweenShape, tweenTiming) = *pair;
		TimeRange firstTimeRange = first.getTimeRange();
		TimeRange secondTimeRange = second.getTimeRange();

		centiseconds tweenStart, tweenDuration;
		switch (tweenTiming) {
		case TweenTiming::Early: {
			tweenDuration = std::min(firstTimeRange.getDuration() / 3, maxTweenDuration);
			tweenStart = firstTimeRange.getEnd() - tweenDuration;
			break;
		}
		case TweenTiming::Centered: {
			tweenDuration = std::min({firstTimeRange.getDuration() / 4, secondTimeRange.getDuration() / 4, maxTweenDuration});
			tweenStart = firstTimeRange.getEnd() - tweenDuration / 2;
			break;
		}
		case TweenTiming::Late: {
			tweenDuration = std::min(secondTimeRange.getDuration() / 3, maxTweenDuration);
			tweenStart = secondTimeRange.getStart();
			break;
		}
		}

		if (tweenDuration < minTweenDuration) return;

		result.set(tweenStart, tweenStart + tweenDuration, tweenShape);
	});

	return result;
}
