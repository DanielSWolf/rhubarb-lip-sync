module;

#include <boost/range/adaptor/transformed.hpp>

#include "core/phone.h"
#include "core/shape.h"
#include "time/bounded-timeline.h"
#include "time/continuous-timeline.h"
#include "time/time-range.h"

export module rhubarb.animation:shape_rule;

import :animation_rules;

using boost::optional;
using boost::adaptors::transformed;

template <typename T, bool AutoJoin>
ContinuousTimeline<optional<T>, AutoJoin> boundedTimelinetoContinuousOptional(
    const BoundedTimeline<T, AutoJoin>& timeline
) {
    return {
        timeline.getRange(),
        boost::none,
        timeline | transformed([](const Timed<T>& timedValue) {
            return Timed<optional<T>>(timedValue.getTimeRange(), timedValue.getValue());
        })
    };
}

export struct ShapeRule {
    ShapeSet shapeSet;
    optional<Phone> phone;
    TimeRange phoneTiming;

    ShapeRule(ShapeSet shapeSet, optional<Phone> phone, TimeRange phoneTiming) :
        shapeSet(std::move(shapeSet)),
        phone(std::move(phone)),
        phoneTiming(phoneTiming) {}

    static ShapeRule getInvalid() {
        return {{}, boost::none, {0_cs, 0_cs}};
    }

    bool operator==(const ShapeRule& rhs) const {
        return shapeSet == rhs.shapeSet && phone == rhs.phone && phoneTiming == rhs.phoneTiming;
    }

    bool operator!=(const ShapeRule& rhs) const {
        return !operator==(rhs);
    }

    bool operator<(const ShapeRule& rhs) const {
        return shapeSet < rhs.shapeSet
            || phone < rhs.phone
            || phoneTiming.getStart() < rhs.phoneTiming.getStart()
            || phoneTiming.getEnd() < rhs.phoneTiming.getEnd();
    }
};

// Returns shape rules for an entire timeline of phones.
ContinuousTimeline<ShapeRule> getShapeRules(const BoundedTimeline<Phone>& phones) {
    // Convert to continuous timeline so that silences aren't skipped when iterating
    auto continuousPhones = boundedTimelinetoContinuousOptional(phones);

    // Create timeline of shape rules
    ContinuousTimeline<ShapeRule> shapeRules(
        phones.getRange(), {{Shape::X}, boost::none, {0_cs, 0_cs}}
    );
    centiseconds previousDuration = 0_cs;
    for (const auto& timedPhone : continuousPhones) {
        optional<Phone> phone = timedPhone.getValue();
        const centiseconds duration = timedPhone.getDuration();

        if (phone) {
            // Animate one phone
            Timeline<ShapeSet> phoneShapeSets = getShapeSets(*phone, duration, previousDuration);

            // Result timing is relative to phone. Make absolute.
            phoneShapeSets.shift(timedPhone.getStart());

            // Copy to timeline.
            // Later shape sets may overwrite earlier ones if overlapping.
            for (const auto& timedShapeSet : phoneShapeSets) {
                shapeRules.set(
                    timedShapeSet.getTimeRange(),
                    ShapeRule(timedShapeSet.getValue(), phone, timedPhone.getTimeRange())
                );
            }
        }

        previousDuration = duration;
    }

    return shapeRules;
}
