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

Timeline<ShapeSet> animatePhone(optional<Phone> phone, centiseconds duration, centiseconds previousDuration) {
	// Returns a timeline with a single shape set
	auto single = [&](ShapeSet value) {
		return Timeline<ShapeSet> { { 0_cs, duration, value } };
	};

	// Returns a timeline with two shape sets, timed as a diphthong
	auto diphthong = [&](ShapeSet first, ShapeSet second) {
		centiseconds firstDuration = duration_cast<centiseconds>(duration * 0.6);
		return Timeline<ShapeSet> {
			{ 0_cs, firstDuration, first },
			{ firstDuration, duration, second }
		};
	};

	// Returns a timeline with two shape sets, timed as a plosive
	auto plosive = [&](ShapeSet first, ShapeSet second) {
		centiseconds maxOcclusionDuration = 12_cs;
		centiseconds leftOverlap = clamp(previousDuration / 2, 4_cs, maxOcclusionDuration);
		centiseconds rightOverlap = min(duration, maxOcclusionDuration - leftOverlap);
		return Timeline<ShapeSet> {
			{ -leftOverlap, rightOverlap, first },
			{ rightOverlap, duration, second }
		};
	};

	if (!phone)				return single({ X });

	switch (*phone) {
	case Phone::AO:			return single({ E });
	case Phone::AA:			return single({ D });
	case Phone::IY:			return single({ B });
	case Phone::UW:			return single({ F });
	case Phone::EH:			return duration < 20_cs ? single({ C }) : single({ D });
	case Phone::IH:			return single({ B });
	case Phone::UH:			return single({ E });
	case Phone::AH:			return single({ C });
	case Phone::Schwa:		return single({ { B, C, D, E, F } });
	case Phone::AE:			return single({ D });
	case Phone::EY:			return duration < 20_cs ? diphthong({ C }, { B }) : diphthong({ D }, { B });
	case Phone::AY:			return diphthong({ D }, { B });
	case Phone::OW:			return diphthong({ E }, { F });
	case Phone::AW:			return diphthong({ D }, { F });
	case Phone::OY:			return diphthong({ F }, { B });
	case Phone::ER:			return duration < 7_cs ? single({ B }) : single({ E });

	case Phone::P:
	case Phone::B:			return plosive({ A }, { A, B, C, D, E, F, G, H, X });
	case Phone::T:
	case Phone::D:
	case Phone::K:			return single({ B, F });
	case Phone::G:			return single({ B, C, E, F });
	case Phone::CH:
	case Phone::JH:			return single({ B, F });
	case Phone::F:
	case Phone::V:			return single({ G });
	case Phone::TH:
	case Phone::DH:
	case Phone::S:
	case Phone::Z:
	case Phone::SH:
	case Phone::ZH:			return single({ B, F });
	case Phone::HH:			return single({ B, C, D, E, F });
	case Phone::M:			return single({ A });
	case Phone::N:			return single({ B, C, F });
	case Phone::NG:			return single({ B, C, D, E, F });
	case Phone::L:			return single({ E, F, H });
	case Phone::R:			return single({ B, F });
	case Phone::Y:			return single({ B });
	case Phone::W:			return single({ F });

	case Phone::Breath:
	case Phone::Cough:
	case Phone::Smack:		return single({ C });
	case Phone::Noise:		return single({ B });

	default:				throw std::invalid_argument("Unexpected phone.");
	}
}
