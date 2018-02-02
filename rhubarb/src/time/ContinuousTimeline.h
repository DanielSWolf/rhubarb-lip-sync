#pragma once

#include "BoundedTimeline.h"

template<typename T, bool AutoJoin = false>
class ContinuousTimeline : public BoundedTimeline<T, AutoJoin> {

public:
	ContinuousTimeline(TimeRange range, T defaultValue) :
		BoundedTimeline<T, AutoJoin>(range),
		defaultValue(defaultValue)
	{
		// Virtual function call in constructor. Derived constructors shouldn't call this one!
		ContinuousTimeline::clear(range);
	}

	template<typename InputIterator>
	ContinuousTimeline(TimeRange range, T defaultValue, InputIterator first, InputIterator last) :
		ContinuousTimeline(range, defaultValue)
	{
		// Virtual function calls in constructor. Derived constructors shouldn't call this one!
		for (auto it = first; it != last; ++it) {
			ContinuousTimeline::set(*it);
		}
	}

	template<typename collection_type>
	ContinuousTimeline(TimeRange range, T defaultValue, collection_type collection) :
		ContinuousTimeline(range, defaultValue, collection.begin(), collection.end())
	{}

	ContinuousTimeline(TimeRange range, T defaultValue, std::initializer_list<Timed<T>> initializerList) :
		ContinuousTimeline(range, defaultValue, initializerList.begin(), initializerList.end())
	{}

	using BoundedTimeline<T, AutoJoin>::clear;

	void clear(const TimeRange& range) override {
		BoundedTimeline<T, AutoJoin>::set(Timed<T>(range, defaultValue));
	}

private:
	T defaultValue;
};

template<typename T>
using JoiningContinuousTimeline = ContinuousTimeline<T, true>;
