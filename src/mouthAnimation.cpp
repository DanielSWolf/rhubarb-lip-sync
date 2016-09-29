#include "mouthAnimation.h"
#include "logging.h"
#include <unordered_set>
#include <unordered_map>
#include <boost/algorithm/clamp.hpp>
#include "Viseme.h"

using std::map;
using std::unordered_set;
using std::unordered_map;
using std::vector;
using boost::optional;
using std::chrono::duration_cast;
using boost::algorithm::clamp;
using std::pair;
using std::tuple;

constexpr Shape A = Shape::A;
constexpr Shape B = Shape::B;
constexpr Shape C = Shape::C;
constexpr Shape D = Shape::D;
constexpr Shape E = Shape::E;
constexpr Shape F = Shape::F;
constexpr Shape G = Shape::G;
constexpr Shape H = Shape::H;
constexpr Shape X = Shape::X;

Timeline<Viseme> animate(optional<Phone> phone, centiseconds duration, centiseconds previousPhoneDuration) {
	auto single = [&](Viseme viseme) {
		return Timeline<Viseme>{
			{ 0_cs, duration, viseme }
		};
	};

	auto diphtong = [&](Viseme first, Viseme second) {
		centiseconds firstDuration = duration_cast<centiseconds>(duration * 0.6);
		return Timeline<Viseme>{
			{ 0_cs, firstDuration, first },
			{ firstDuration, duration, second }
		};
	};

	auto bilabialStop = [&]() {
		centiseconds maxDuration = 12_cs;
		centiseconds leftOverlap = clamp(previousPhoneDuration / 2, 4_cs, maxDuration);
		centiseconds rightOverlap = min(duration, maxDuration - leftOverlap);
		return Timeline<Viseme>{
			{ -leftOverlap, rightOverlap, { A } },
			{ rightOverlap, duration, {{ B, C, D, E, F }} }
		};
	};

	if (!phone)				return single({ X });

	switch (*phone) {
	case Phone::AO:			return single({ E });
	case Phone::AA:			return single({ D });
	case Phone::IY:			return single({ B });
	case Phone::UW:			return single({ F });
	case Phone::EH:			return single({ { C }, 20_cs, { D } });
	case Phone::IH:			return single({ B });
	case Phone::UH:			return single({ E });
	case Phone::AH:			return single({ { B, C, D, E, F }, 6_cs, { C } }); // Heuristic: < 6_cs is schwa
	case Phone::AE:			return single({ D });
	case Phone::EY:			return diphtong({ { C }, 20_cs, { D } }, { B });
	case Phone::AY:			return diphtong({ D }, { B });
	case Phone::OW:			return diphtong({ E }, { F });
	case Phone::AW:			return diphtong({ D }, { F });
	case Phone::OY:			return diphtong({ F }, { B });
	case Phone::ER:			return single({ { B }, 7_cs, { E } });
	
	case Phone::P:
	case Phone::B:			return bilabialStop();
	case Phone::T:
	case Phone::D:
	case Phone::K:			return single({ { B, B, B, B, F } });
	case Phone::G:			return single({ { B, C, C, E, F } });
	case Phone::CH:
	case Phone::JH:			return single({ { B, B, B, B, F } });
	case Phone::F:
	case Phone::V:			return single({ G });
	case Phone::TH:
	case Phone::DH:
	case Phone::S:
	case Phone::Z:
	case Phone::SH:
	case Phone::ZH:			return single({ { B, B, B, B, F } });
	case Phone::HH:			return single({ { B, C, D, E, F } });
	case Phone::M:			return single({ A });
	case Phone::N:			return single({ { B, C, C, C, F } });
	case Phone::NG:			return single({ { B, C, D, E, F } });
	case Phone::L:			return single({ { H, H, H, E, F } });
	case Phone::R:			return single({ { B, B, B, B, F } });
	case Phone::Y:			return single({ B });
	case Phone::W:			return single({ F });
	
	case Phone::Breath:
	case Phone::Cough:
	case Phone::Smack:		return single({ C });
	case Phone::Noise:	return single({ B });

	default:
		throw std::invalid_argument("Unexpected phone.");
	}
}

enum class TweenTiming {
	Early,
	Centered,
	Late
};

optional<pair<Shape, TweenTiming>> getTween(Shape first, Shape second) {
	static const map<pair<Shape, Shape>, pair<Shape, TweenTiming>> lookup {
		{ { A, D }, { C, TweenTiming::Late } },			{ { D, A },{ C, TweenTiming::Early } },
		{ { B, D }, { C, TweenTiming::Centered } },		{ { D, B },{ C, TweenTiming::Centered } },
		{ { G, D }, { C, TweenTiming::Late } },			{ { D, G },{ C, TweenTiming::Early } },
		{ { X, D }, { C, TweenTiming::Early } },		{ { D, X },{ C, TweenTiming::Late } },
		{ { C, F }, { E, TweenTiming::Centered } },		{ { F, C },{ E, TweenTiming::Centered } },
		{ { D, F }, { E, TweenTiming::Centered } },		{ { F, D },{ E, TweenTiming::Centered } },
		{ { H, F }, { E, TweenTiming::Late } },			{ { F, H },{ E, TweenTiming::Early } },
	};
	auto it = lookup.find({ first, second });
	return it != lookup.end() ? it->second : optional<pair<Shape, TweenTiming>>();
}

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
	for (const auto& timedShape : shapes) {
		if (timedShape.getValue() != X) continue;
		if (timedShape.getStart() == 0_cs || timedShape.getEnd() == shapes.getRange().getEnd()) continue;

		const centiseconds maxPausedOpenMouthDuration = 35_cs;
		const TimeRange timeRange = timedShape.getTimeRange();
		if (timeRange.getDuration() <= maxPausedOpenMouthDuration) {
			result.set(timeRange, B);
		}
	}

	// Keep mouth open into pause if it just opened
	for_each_adjacent(shapes.begin(), shapes.end(), [&](const Timed<Shape>& secondLast, const Timed<Shape>& last, const Timed<Shape>& pause) {
		if (pause.getValue() != X) return;

		centiseconds lastDuration = last.getDuration();
		const centiseconds minOpenDuration = 20_cs;
		if (isClosed(secondLast.getValue()) && !isClosed(last.getValue()) && lastDuration < minOpenDuration) {
			const centiseconds minSpillDuration = 20_cs;
			centiseconds spillDuration = std::min(minSpillDuration, pause.getDuration());
			result.set(pause.getStart(), pause.getStart() + spillDuration, B);
		}
	});

	return result;
}

ContinuousTimeline<Shape> animate(const BoundedTimeline<Phone> &phones) {
	// Convert phones to continuous timeline so that silences aren't skipped when iterating
	ContinuousTimeline<optional<Phone>> continuousPhones(phones.getRange(), boost::none);
	for (const auto& timedPhone : phones) {
		continuousPhones.set(timedPhone.getTimeRange(), timedPhone.getValue());
	}

	// Create timeline of visemes
	ContinuousTimeline<Viseme> visemes(phones.getRange(), { X });
	centiseconds previousPhoneDuration = 0_cs;
	for (const auto& timedPhone : continuousPhones) {
		// Animate one phone
		optional<Phone> phone = timedPhone.getValue();
		centiseconds duration = timedPhone.getDuration();
		Timeline<Viseme> phoneVisemes = animate(phone, duration, previousPhoneDuration);

		// Result timing is relative to phone. Make absolute.
		phoneVisemes.shift(timedPhone.getStart());

		// Copy to viseme timeline
		for (const auto& timedViseme : phoneVisemes) {
			visemes.set(timedViseme);
		}

		previousPhoneDuration = duration;
	}

	// Create timeline of shapes.
	// Iterate visemes in *reverse* order so we always know what shape will follow.
	ContinuousTimeline<Shape> shapes(phones.getRange(), X);
	Shape lastShape = X;
	for (auto it = visemes.rbegin(); it != visemes.rend(); ++it) {
		Viseme viseme = it->getValue();

		// Convert viseme to phone
		Shape shape = viseme.getShape(it->getDuration(), lastShape);
		shapes.set(it->getTimeRange(), shape);

		lastShape = shape;
	}

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
		logging::logTimedEvent("shape", timedShape);
	}

	return shapes;
}
