#include "mouthAnimation.h"
#include "logging.h"
#include <unordered_set>
#include <unordered_map>
#include <array>

using std::map;
using std::unordered_set;
using std::unordered_map;
using std::vector;
using boost::optional;

using AnimationResult = Timeline<Shape>;

AnimationResult animateFixedSound(Shape shape, centiseconds duration) {
	return AnimationResult{ {centiseconds::zero(), duration, shape} };
}

// Diphtong vowels
AnimationResult animateDiphtong(Shape first, Shape second, centiseconds duration) {
	return AnimationResult{
		{ centiseconds::zero(), duration, first },
		{ duration / 2, duration, second }
	};
}

// P, B
AnimationResult animateBilabialStop(centiseconds duration, centiseconds leftDuration, optional<Shape> rightShape) {
	Shape openShape = rightShape.value_or(Shape::B);
	if (openShape == Shape::A) {
		openShape = Shape::B;
	}
	return AnimationResult{
		{ -leftDuration / 4, centiseconds::zero(), Shape::A },
		{ centiseconds::zero(), duration, openShape }
	};
}

// Sounds with no fixed mouth position.
// Mapping specifies the shape to use for every right shape.
AnimationResult animateFlexibleSound(std::array<Shape, 7> mapping, centiseconds duration, optional<Shape> rightShape) {
	constexpr int mapSize = std::tuple_size<decltype(mapping)>::value;
	static_assert(static_cast<int>(Shape::EndSentinel) == mapSize, "Shape definition has changed.");

	Shape right = rightShape.value_or(Shape::A);
	Shape shape = mapping[static_cast<int>(right)];
	return AnimationResult{ { centiseconds::zero(), duration, shape } };
}

AnimationResult animate(optional<Phone> phone, centiseconds duration, centiseconds leftDuration, optional<Shape> rightShape) {
	constexpr Shape A = Shape::A;
	constexpr Shape B = Shape::B;
	constexpr Shape C = Shape::C;
	constexpr Shape D = Shape::D;
	constexpr Shape E = Shape::E;
	constexpr Shape F = Shape::F;
	constexpr Shape G = Shape::G;

	if (!phone) {
		return animateFixedSound(A, duration);
	}

	switch (*phone) {
	case Phone::Unknown:	return animateFixedSound(B, duration);
	case Phone::AO:			return animateFixedSound(E, duration);
	case Phone::AA:			return animateFixedSound(D, duration);
	case Phone::IY:			return animateFixedSound(B, duration);
	case Phone::UW:			return animateFixedSound(F, duration);
	case Phone::EH:			return animateFixedSound(C, duration);
	case Phone::IH:			return animateFixedSound(B, duration);
	case Phone::UH:			return animateFixedSound(E, duration);
	case Phone::AH:			return animateFixedSound(C, duration);
	case Phone::AE:			return animateFixedSound(D, duration);
	case Phone::EY:			return animateDiphtong(C, B, duration);
	case Phone::AY:			return animateDiphtong(D, B, duration);
	case Phone::OW:			return animateDiphtong(E, F, duration);
	case Phone::AW:			return animateDiphtong(D, F, duration);
	case Phone::OY:			return animateDiphtong(E, B, duration);
	case Phone::ER:			return animateFixedSound(E, duration);
	case Phone::P:
	case Phone::B:			return animateBilabialStop(duration, leftDuration, rightShape);
	case Phone::T:
	case Phone::D:
	case Phone::K:			return animateFlexibleSound({ B, B, B, B, B, F, B }, duration, rightShape);
	case Phone::G:			return animateFlexibleSound({ B, B, C, C, E, F, B }, duration, rightShape);
	case Phone::CH:
	case Phone::JH:			return animateFlexibleSound({ B, B, B, B, B, F, B }, duration, rightShape);
	case Phone::F:
	case Phone::V:			return animateFixedSound(G, duration);
	case Phone::TH:
	case Phone::DH:
	case Phone::S:
	case Phone::Z:
	case Phone::SH:
	case Phone::ZH:			return animateFlexibleSound({ B, B, B, B, B, F, B }, duration, rightShape);
	case Phone::HH:			return animateFlexibleSound({ B, B, C, D, E, F, B }, duration, rightShape);
	case Phone::M:			return animateFixedSound(A, duration);
	case Phone::N:			return animateFlexibleSound({ B, B, C, C, C, F, B }, duration, rightShape);
	case Phone::NG:
	case Phone::L:			return animateFlexibleSound({ B, B, C, D, E, F, B }, duration, rightShape);
	case Phone::R:			return animateFlexibleSound({ B, B, B, B, B, F, B }, duration, rightShape);
	case Phone::Y:			return animateFixedSound(B, duration);
	case Phone::W:			return animateFixedSound(F, duration);
	default:
		throw std::invalid_argument("Unexpected phone.");
	}
}

ContinuousTimeline<Shape> animate(const BoundedTimeline<Phone> &phones) {
	// Convert phones to continuous timeline so that silences show up when iterating
	ContinuousTimeline<optional<Phone>> continuousPhones(phones.getRange(), boost::none);
	for (const auto& timedPhone : phones) {
		continuousPhones.set(timedPhone.getTimeRange(), timedPhone.getValue());
	}

	ContinuousTimeline<Shape> shapes(phones.getRange(), Shape::A);

	// Iterate phones in *reverse* order so we can access the right-hand result
	optional<Shape> lastShape;
	centiseconds cutoff = shapes.getRange().getEnd();
	for (auto it = continuousPhones.rbegin(); it != continuousPhones.rend(); ++it) {
		// Animate one phone
		optional<Phone> phone = it->getValue();
		centiseconds duration = it->getTimeRange().getLength();
		centiseconds leftDuration = std::next(it) != continuousPhones.rend()
			? std::next(it)->getTimeRange().getLength()
			: centiseconds::zero();
		Timeline<Shape> result = animate(phone, duration, leftDuration, lastShape);

		// Result timing is relative to phone. Make absolute.
		result.shift(it->getStart());

		// New shapes must not overwrite existing shapes
		result.clear(cutoff, shapes.getRange().getEnd());

		// Copy to target timeline
		for (const auto& timedShape : result) {
			shapes.set(timedShape);
		}

		lastShape = result.empty() ? optional<Shape>() : result.begin()->getValue();
		if (!result.empty()) {
			cutoff = result.begin()->getStart();
		}
	}

	for (const auto& timedShape : shapes) {
		logging::logTimedEvent("shape", timedShape);
	}

	return shapes;
}
