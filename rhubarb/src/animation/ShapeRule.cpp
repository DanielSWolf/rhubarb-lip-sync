#include "ShapeRule.h"
#include <boost/range/adaptor/transformed.hpp>
#include "time/ContinuousTimeline.h"

using boost::optional;
using boost::adaptors::transformed;

template<typename T, bool AutoJoin>
ContinuousTimeline<optional<T>, AutoJoin> boundedTimelinetoContinuousOptional(const BoundedTimeline<T, AutoJoin>& timeline) {
	return{
		timeline.getRange(), boost::none,
		timeline | transformed([](const Timed<T>& timedValue) { return Timed<optional<T>>(timedValue.getTimeRange(), timedValue.getValue()); })
	};
}

ShapeRule::ShapeRule(const ShapeSet& shapeSet, const optional<Phone>& phone, TimeRange phoneTiming) :
	shapeSet(shapeSet),
	phone(phone),
	phoneTiming(phoneTiming)
{}

ShapeRule ShapeRule::getInvalid() {
	return {{}, boost::none,{0_cs, 0_cs}};
}

bool ShapeRule::operator==(const ShapeRule& rhs) const {
	return shapeSet == rhs.shapeSet && phone == rhs.phone && phoneTiming == rhs.phoneTiming;
}

bool ShapeRule::operator!=(const ShapeRule& rhs) const {
	return !operator==(rhs);
}

bool ShapeRule::operator<(const ShapeRule& rhs) const {
	return shapeSet < rhs.shapeSet
		|| phone < rhs.phone
		|| phoneTiming.getStart() < rhs.phoneTiming.getStart()
		|| phoneTiming.getEnd() < rhs.phoneTiming.getEnd();
}

ContinuousTimeline<ShapeRule> getShapeRules(const BoundedTimeline<Phone>& phones) {
	// Convert to continuous timeline so that silences aren't skipped when iterating
	auto continuousPhones = boundedTimelinetoContinuousOptional(phones);

	// Create timeline of shape rules
	ContinuousTimeline<ShapeRule> shapeRules(phones.getRange(), {{Shape::X}, boost::none, {0_cs, 0_cs}});
	centiseconds previousDuration = 0_cs;
	for (const auto& timedPhone : continuousPhones) {
		optional<Phone> phone = timedPhone.getValue();
		centiseconds duration = timedPhone.getDuration();

		if (phone) {
			// Animate one phone
			Timeline<ShapeSet> phoneShapeSets = getShapeSets(*phone, duration, previousDuration);

			// Result timing is relative to phone. Make absolute.
			phoneShapeSets.shift(timedPhone.getStart());

			// Copy to timeline.
			// Later shape sets may overwrite earlier ones if overlapping.
			for (const auto& timedShapeSet : phoneShapeSets) {
				shapeRules.set(timedShapeSet.getTimeRange(), ShapeRule(timedShapeSet.getValue(), phone, timedPhone.getTimeRange()));
			}
		}

		previousDuration = duration;
	}

	return shapeRules;
}
