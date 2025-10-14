module;

#include <array>
#include <boost/algorithm/clamp.hpp>
#include <set>

#include "core/phone.h"
#include "core/shape.h"
#include "time/timeline.h"

export module rhubarb.animation:animation_rules;

using boost::optional;
using boost::algorithm::clamp;
using std::array;
using std::map;
using std::pair;
using std::to_array;
using std::chrono::duration_cast;

using enum Shape;

constexpr size_t shapeValueCount = static_cast<size_t>(Shape::EndSentinel);

// Returns the basic shape (A-F) that most closely resembles the specified shape.
Shape getBasicShape(Shape shape) {
    static constexpr array<Shape, shapeValueCount> basicShapes =
        to_array({A, B, C, D, E, F, A, C, A});
    return basicShapes[static_cast<size_t>(shape)];
}

// Returns the mouth shape that results from relaxing the specified shape.
Shape relax(Shape shape) {
    static constexpr array<Shape, shapeValueCount> relaxedShapes =
        to_array({A, B, B, C, C, B, X, B, X});
    return relaxedShapes[static_cast<size_t>(shape)];
}

// Gets the shape from a non-empty set of shapes that most closely resembles a reference shape.
Shape getClosestShape(Shape reference, ShapeSet shapes) {
    if (shapes.empty()) {
        throw std::invalid_argument("Cannot select from empty set of shapes.");
    }

    // A matrix that for each shape contains all shapes in ascending order of effort required to
    // move to them
    static constexpr array<array<Shape, shapeValueCount>, shapeValueCount> effortMatrix = to_array({
        /* A */ to_array({A, X, G, B, C, H, E, D, F}),
        /* B */ to_array({B, G, A, X, C, H, E, D, F}),
        /* C */ to_array({C, H, B, G, D, A, X, E, F}),
        /* D */ to_array({D, C, H, B, G, A, X, E, F}),
        /* E */ to_array({E, C, H, B, G, A, X, D, F}),
        /* F */ to_array({F, B, G, A, X, C, H, E, D}),
        /* G */ to_array({G, A, B, C, H, X, E, D, F}),
        /* H */ to_array({H, C, B, G, D, A, X, E, F}), // Like C
        /* X */ to_array({X, A, G, B, C, H, E, D, F})  // Like A
    });

    auto& closestShapes = effortMatrix.at(static_cast<size_t>(reference));
    for (Shape closestShape : closestShapes) {
        if (shapes.find(closestShape) != shapes.end()) {
            return closestShape;
        }
    }

    throw std::invalid_argument("Unable to find closest shape.");
}

// Indicates how to time a tween between two mouth shapes
enum class TweenTiming {
    // Tween should end at the original transition
    Early,

    // Tween should overlap both original mouth shapes equally
    Centered,

    // Tween should begin at the original transition
    Late
};

// Returns the tween shape and timing to use to transition between the specified two mouth shapes.
optional<pair<Shape, TweenTiming>> getTween(Shape first, Shape second) {
    // Note that most of the following rules work in one direction only.
    // That's because in animation, the mouth should usually "pop" open without inbetweens,
    // then close slowly.
    static const map<pair<Shape, Shape>, pair<Shape, TweenTiming>> lookup{
        {{D, A}, {C, TweenTiming::Early}},
        {{D, B}, {C, TweenTiming::Centered}},
        {{D, G}, {C, TweenTiming::Early}},
        {{D, X}, {C, TweenTiming::Late}},
        {{C, F}, {E, TweenTiming::Centered}},
        {{F, C}, {E, TweenTiming::Centered}},
        {{D, F}, {E, TweenTiming::Centered}},
        {{H, F}, {E, TweenTiming::Late}},
        {{F, H}, {E, TweenTiming::Early}}
    };
    const auto it = lookup.find({first, second});
    return it != lookup.end() ? it->second : optional<pair<Shape, TweenTiming>>();
}

// Returns the shape set(s) to use for a given phone.
// The resulting timeline will always cover the entire duration of the phone (starting at 0 cs).
// It may extend into the negative time range if animation is required prior to the sound being
// heard.
Timeline<ShapeSet> getShapeSets(Phone phone, centiseconds duration, centiseconds previousDuration) {
    // Returns a timeline with a single shape set
    const auto single = [duration](ShapeSet value) {
        return Timeline<ShapeSet>{{0_cs, duration, value}};
    };

    // Returns a timeline with two shape sets, timed as a diphthong
    const auto diphthong = [duration](ShapeSet first, ShapeSet second) {
        const centiseconds firstDuration = duration_cast<centiseconds>(duration * 0.6);
        return Timeline<ShapeSet>{{0_cs, firstDuration, first}, {firstDuration, duration, second}};
    };

    // Returns a timeline with two shape sets, timed as a plosive
    const auto plosive = [duration, previousDuration](ShapeSet first, ShapeSet second) {
        const centiseconds minOcclusionDuration = 4_cs;
        const centiseconds maxOcclusionDuration = 12_cs;
        const centiseconds occlusionDuration =
            clamp(previousDuration / 2, minOcclusionDuration, maxOcclusionDuration);
        return Timeline<ShapeSet>{{-occlusionDuration, 0_cs, first}, {0_cs, duration, second}};
    };

    // Returns the result of `getShapeSets` when called with identical arguments
    // except for a different phone.
    const auto like = [duration, previousDuration](Phone referencePhone) {
        return getShapeSets(referencePhone, duration, previousDuration);
    };

    static const ShapeSet any{A, B, C, D, E, F, G, H, X};
    static const ShapeSet anyOpen{B, C, D, E, F, G, H};

    // Note:
    // The shapes {A, B, G, X} are very similar. You should avoid regular shape sets containing more
    // than one of these shapes.
    // Otherwise, the resulting shape may be more or less random and might not be a good fit.
    // As an exception, a very flexible rule may contain *all* these shapes.

    switch (phone) {
        case Phone::AO: return single({E});
        case Phone::AA: return single({D});
        case Phone::IY: return single({B});
        case Phone::UW: return single({F});
        case Phone::EH: return single({C});
        case Phone::IH: return single({B});
        case Phone::UH: return single({F});
        case Phone::AH: return duration < 20_cs ? single({C}) : single({D});
        case Phone::Schwa: return single({B, C});
        case Phone::AE: return single({C});
        case Phone::EY: return diphthong({C}, {B});
        case Phone::AY: return duration < 20_cs ? diphthong({C}, {B}) : diphthong({D}, {B});
        case Phone::OW: return diphthong({E}, {F});
        case Phone::AW: return duration < 30_cs ? diphthong({C}, {E}) : diphthong({D}, {E});
        case Phone::OY: return diphthong({E}, {B});
        case Phone::ER: return duration < 7_cs ? like(Phone::Schwa) : single({E});

        case Phone::P:
        case Phone::B: return plosive({A}, any);
        case Phone::T:
        case Phone::D: return plosive({B, F}, anyOpen);
        case Phone::K:
        case Phone::G: return plosive({B, C, E, F, H}, anyOpen);
        case Phone::CH:
        case Phone::JH: return single({B, F});
        case Phone::F:
        case Phone::V: return single({G});
        case Phone::TH:
        case Phone::DH:
        case Phone::S:
        case Phone::Z:
        case Phone::SH:
        case Phone::ZH: return single({B, F});
        case Phone::HH: return single(any); // think "m-hm"
        case Phone::M: return single({A});
        case Phone::N: return single({B, C, F, H});
        case Phone::NG: return single({B, C, E, F});
        case Phone::L: return duration < 20_cs ? single({B, E, F, H}) : single({H});
        case Phone::R: return single({B, E, F});
        case Phone::Y: return single({B, C, F});
        case Phone::W: return single({F});

        case Phone::Breath:
        case Phone::Cough:
        case Phone::Smack: return single({C});
        case Phone::Noise: return single({B});

        default: throw std::invalid_argument("Unexpected phone.");
    }
}
