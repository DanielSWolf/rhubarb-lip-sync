#pragma once

#include "Phone.h"
#include "animationRules.h"
#include "BoundedTimeline.h"
#include "ContinuousTimeline.h"

struct ShapeRule {
	ShapeSet shapeSet;
	boost::optional<Phone> phone;
	TimeRange phoneTiming;

	ShapeRule(const ShapeSet& shapeSet, const boost::optional<Phone>& phone, TimeRange phoneTiming);

	static ShapeRule getInvalid();

	bool operator==(const ShapeRule&) const;
	bool operator!=(const ShapeRule&) const;
	bool operator<(const ShapeRule&) const;
};

// Returns shape rules for an entire timeline of phones.
ContinuousTimeline<ShapeRule> getShapeRules(const BoundedTimeline<Phone>& phones);
