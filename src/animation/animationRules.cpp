#include "animationRules.h"
#include <boost/algorithm/clamp.hpp>

using std::chrono::duration_cast;
using boost::algorithm::clamp;
using boost::optional;

constexpr Shape A = Shape::A;
constexpr Shape B = Shape::B;
constexpr Shape C = Shape::C;
constexpr Shape D = Shape::D;
constexpr Shape E = Shape::E;
constexpr Shape F = Shape::F;
constexpr Shape G = Shape::G;
constexpr Shape H = Shape::H;
constexpr Shape X = Shape::X;

ShapeRule::ShapeRule(const ShapeSet& regularShapes, const ShapeSet& alternativeShapes) :
	regularShapes(regularShapes),
	alternativeShapes(alternativeShapes)
{}

Timeline<ShapeRule> animatePhone(optional<Phone> phone, centiseconds duration, centiseconds previousDuration) {
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

	// Returns the result of `animatePhone` when called with identical arguments
	// except for a different phone.
	auto like = [duration, previousDuration](optional<Phone> referencePhone) {
		return animatePhone(referencePhone, duration, previousDuration);
	};

	static const ShapeRule any{{A, B, C, D, E, F, G, H, X}};

	if (!phone) {
		return single({{X}});
	}

	switch (*phone) {
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
	case Phone::D:			return plosive({{B, C, F, G, H}}, any);
	case Phone::K:
	case Phone::G:			return plosive({{B, C, E, F, G, H}}, any);
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
	case Phone::N:			return single({{B, C, F, G, H}});
	case Phone::NG:			return single({{B, C, E, F, G}});
	case Phone::L:			return duration < 20_cs ? single({{B, C, D, E, F, G, H}}) : single({{H}});
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
