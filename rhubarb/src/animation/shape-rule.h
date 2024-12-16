#pragma once

#include "animation-rules.h"
#include "core/phone.h"
#include "time/bounded-timeline.h"
#include "time/continuous-timeline.h"
#include "time/time-range.h"

struct ShapeRule {
    ShapeSet shapeSet;
    boost::optional<Phone> phone;
    TimeRange phoneTiming;

    ShapeRule(ShapeSet shapeSet, boost::optional<Phone> phone, TimeRange phoneTiming);

    static ShapeRule getInvalid();

    bool operator==(const ShapeRule&) const;
    bool operator!=(const ShapeRule&) const;
    bool operator<(const ShapeRule&) const;
};

// Returns shape rules for an entire timeline of phones.
ContinuousTimeline<ShapeRule> getShapeRules(const BoundedTimeline<Phone>& phones);
