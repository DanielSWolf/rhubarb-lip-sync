#include "animationRules.h"
#include <boost/algorithm/clamp.hpp>
#include "shapeShorthands.h"
#include "array.h"

using std::chrono::duration_cast;
using boost::algorithm::clamp;
using boost::optional;
using std::array;
using std::pair;
using std::map;

constexpr size_t shapeValueCount = static_cast<size_t>(Shape::EndSentinel);

Shape getBasicShape(Shape shape) {
	static constexpr array<Shape, shapeValueCount> basicShapes = make_array(A, B, C, D, E, F, B, C, A);
	return basicShapes[static_cast<size_t>(shape)];
}

Shape relax(Shape shape) {
	static constexpr array<Shape, shapeValueCount> relaxedShapes = make_array(A, B, B, C, C, B, X, B, X);
	return relaxedShapes[static_cast<size_t>(shape)];
}

Shape getRelaxedBridge(Shape lhs, Shape rhs) {
	return lhs == rhs ? lhs : relax(lhs);
}

Shape getClosestShape(Shape reference, ShapeSet shapes) {
	if (shapes.empty()) {
		throw std::invalid_argument("Cannot select from empty set of shapes.");
	}

	// A matrix that for each shape contains all shapes in ascending order of effort required to move to them
	constexpr static array<array<Shape, shapeValueCount>, shapeValueCount> effortMatrix = make_array(
		/* A */ make_array(A, X, G, B, C, H, E, D, F),
		/* B */ make_array(B, G, A, X, C, H, E, D, F),
		/* C */ make_array(C, H, B, G, D, A, X, E, F),
		/* D */ make_array(D, C, H, B, G, A, X, E, F),
		/* E */ make_array(E, C, H, B, G, A, X, D, F),
		/* F */ make_array(F, B, G, A, X, C, H, E, D),
		/* G */ make_array(G, B, C, H, A, X, E, D, F),
		/* H */ make_array(H, C, B, G, D, A, X, E, F), // Like C
		/* X */ make_array(X, A, G, B, C, H, E, D, F)  // Like A
	);

	auto& closestShapes = effortMatrix.at(static_cast<size_t>(reference));
	for (Shape closestShape : closestShapes) {
		if (shapes.find(closestShape) != shapes.end()) {
			return closestShape;
		}
	}

	throw std::invalid_argument("Unable to find closest shape.");
}

optional<pair<Shape, TweenTiming>> getTween(Shape first, Shape second) {
	static const map<pair<Shape, Shape>, pair<Shape, TweenTiming>> lookup{
		{{A, D}, {C, TweenTiming::Late}},		{{D, A}, {C, TweenTiming::Early}},
		{{B, D}, {C, TweenTiming::Centered}},	{{D, B}, {C, TweenTiming::Centered}},
		{{G, D}, {C, TweenTiming::Late}},		{{D, G}, {C, TweenTiming::Early}},
		{{X, D}, {C, TweenTiming::Early}},		{{D, X}, {C, TweenTiming::Late}},
		{{C, F}, {E, TweenTiming::Centered}},	{{F, C}, {E, TweenTiming::Centered}},
		{{D, F}, {E, TweenTiming::Centered}},	{{F, D}, {E, TweenTiming::Centered}},
		{{H, F}, {E, TweenTiming::Late}},		{{F, H}, {E, TweenTiming::Early}},
	};
	auto it = lookup.find({first, second});
	return it != lookup.end() ? it->second : optional<pair<Shape, TweenTiming>>();
}

ShapeRule::ShapeRule(const ShapeSet& regularShapes, const ShapeSet& alternativeShapes) :
	regularShapes(regularShapes),
	alternativeShapes(alternativeShapes)
{}

Timeline<ShapeRule> getShapeRules(Phone phone, centiseconds duration, centiseconds previousDuration) {
	// Returns a timeline with a single shape set
	auto single = [duration](ShapeRule value) {
		return Timeline<ShapeRule> {{0_cs, duration, value}};
	};

	// Returns a timeline with two shape sets, timed as a diphthong
	auto diphthong = [duration](ShapeRule first, ShapeRule second) {
		centiseconds firstDuration = duration_cast<centiseconds>(duration * 0.6);
		return Timeline<ShapeRule> {
			{0_cs, firstDuration, first},
			{firstDuration, duration, second}
		};
	};

	// Returns a timeline with two shape sets, timed as a plosive
	auto plosive = [duration, previousDuration](ShapeRule first, ShapeRule second) {
		centiseconds minOcclusionDuration = 4_cs;
		centiseconds maxOcclusionDuration = 12_cs;
		centiseconds occlusionDuration = clamp(previousDuration / 2, minOcclusionDuration, maxOcclusionDuration);
		return Timeline<ShapeRule> {
			{-occlusionDuration, 0_cs, first},
			{0_cs, duration, second}
		};
	};

	// Returns the result of `getShapeRules` when called with identical arguments
	// except for a different phone.
	auto like = [duration, previousDuration](Phone referencePhone) {
		return getShapeRules(referencePhone, duration, previousDuration);
	};

	static const ShapeRule any{{A, B, C, D, E, F, G, H, X}};

	// Note:
	// The shapes {A, B, G, X} are very similar. You should avoid regular shape sets containing more than one of these shapes.
	// Otherwise, the resulting shape may be more or less random and might not be a good fit.
	// As an exception, a very flexible rule may contain *all* these shapes.

	switch (phone) {
	case Phone::AO:			return single({{E}});
	case Phone::AA:			return single({{D}});
	case Phone::IY:			return single({{B}, {C}});
	case Phone::UW:			return single({{F}});
	case Phone::EH:			return duration < 20_cs ? single({{C}}) : single({{D}});
	case Phone::IH:			return single({{B}, {C}});
	case Phone::UH:			return single({{F}});
	case Phone::AH:			return single({{C}});
	case Phone::Schwa:		return single({{B, C, D, E, F}});
	case Phone::AE:			return single({{C}});
	case Phone::EY:			return diphthong({{C}}, {{B}, {C}});
	case Phone::AY:			return duration < 20_cs ? diphthong({{C}}, {{B}, {C}}) : diphthong({{D}}, {{B}, {C}});
	case Phone::OW:			return single({{F}});
	case Phone::AW:			return duration < 20_cs ? diphthong({{C}}, {{F}}) : diphthong({{D}}, {{F}});
	case Phone::OY:			return diphthong({{F}}, {{B}, {C}});
	case Phone::ER:			return duration < 7_cs ? like(Phone::Schwa) : single({{F}});

	case Phone::P:
	case Phone::B:			return plosive({{A}}, any);
	case Phone::T:
	case Phone::D:			return plosive({{B, C, F, H}}, {{B, C, D, E, F, G, H}});
	case Phone::K:
	case Phone::G:			return plosive({{B, C, E, F, H}}, any);
	case Phone::CH:
	case Phone::JH:			return single({{B, F}});
	case Phone::F:
	case Phone::V:			return single({{G}});
	case Phone::TH:
	case Phone::DH:
	case Phone::S:
	case Phone::Z:
	case Phone::SH:
	case Phone::ZH:			return single({{B, F}});
	case Phone::HH:			return single(any);
	case Phone::M:			return single({{A}});
	case Phone::N:			return single({{B, C, F, H}});
	case Phone::NG:			return single({{B, C, E, F}});
	case Phone::L:			return duration < 20_cs ? single({{B, C, D, E, F, H}}) : single({{H}});
	case Phone::R:			return single({{B, C, E, F}});
	case Phone::Y:			return single({{B, C, F}});
	case Phone::W:			return single({{F}});

	case Phone::Breath:
	case Phone::Cough:
	case Phone::Smack:		return single({{C}});
	case Phone::Noise:		return single({{B}});

	default:				throw std::invalid_argument("Unexpected phone.");
	}
}
