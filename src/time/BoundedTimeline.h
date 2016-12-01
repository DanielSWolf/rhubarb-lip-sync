#pragma once

#include "Timeline.h"

template<typename T>
class BoundedTimeline : public Timeline<T> {
	using typename Timeline<T>::time_type;
	using Timeline<T>::equals;

public:
	using typename Timeline<T>::iterator;
	using Timeline<T>::end;

	BoundedTimeline() :
		range(TimeRange::zero())
	{}

	explicit BoundedTimeline(TimeRange range) :
		range(range)
	{}

	template<typename InputIterator>
	BoundedTimeline(TimeRange range, InputIterator first, InputIterator last) :
		range(range)
	{
		for (auto it = first; it != last; ++it) {
			// Virtual function call in constructor. Derived constructors shouldn't call this one!
			BoundedTimeline<T>::set(*it);
		}
	}

	template<typename collection_type>
	BoundedTimeline(TimeRange range, collection_type collection) :
		BoundedTimeline(range, collection.begin(), collection.end())
	{}

	BoundedTimeline(TimeRange range, std::initializer_list<Timed<T>> initializerList) :
		BoundedTimeline(range, initializerList.begin(), initializerList.end())
	{}

	TimeRange getRange() const override {
		return range;
	}

	using Timeline<T>::set;

	iterator set(Timed<T> timedValue) override {
		// Exit if the value's range is completely out of bounds
		if (timedValue.getEnd() <= range.getStart() || timedValue.getStart() >= range.getEnd()) {
			return end();
		}

		// Clip the value's range to bounds
		TimeRange& valueRange = timedValue.getTimeRange();
		valueRange.resize(max(range.getStart(), valueRange.getStart()), min(range.getEnd(), valueRange.getEnd()));

		return Timeline<T>::set(timedValue);
	}

	void shift(time_type offset) override {
		Timeline<T>::shift(offset);
		range.shift(offset);
	}

	bool operator==(const BoundedTimeline& rhs) const {
		return Timeline<T>::equals(rhs) && range == rhs.range;
	}

	bool operator!=(const BoundedTimeline& rhs) const {
		return !operator==(rhs);
	}

private:
	TimeRange range;
};