#include "tweening.h"
#include "animationRules.h"

JoiningContinuousTimeline<Shape> insertTweens(const JoiningContinuousTimeline<Shape>& shapes) {
	centiseconds minTweenDuration = 4_cs;
	centiseconds maxTweenDuration = 8_cs;

	JoiningContinuousTimeline<Shape> result(shapes);

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

		if (tweenDuration < minTweenDuration) continue;

		result.set(tweenStart, tweenStart + tweenDuration, tweenShape);
	}

	return result;
}
