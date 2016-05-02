#pragma once

#include "BoundedTimeline.h"

template<typename T>
class ContinuousTimeline : public BoundedTimeline<T> {

public:
	ContinuousTimeline(TimeRange range, T defaultValue) :
		BoundedTimeline(range),
		defaultValue(defaultValue)
	{
		// Virtual function call in constructor. Derived constructors shouldn't call this one!
		ContinuousTimeline<T>::clear(range);
	}

	template<typename InputIterator>
	ContinuousTimeline(TimeRange range, T defaultValue, InputIterator first, InputIterator last) :
		ContinuousTimeline(range, defaultValue)
	{
		// Virtual function calls in constructor. Derived constructors shouldn't call this one!
		for (auto it = first; it != last; ++it) {
			ContinuousTimeline<T>::set(*it);
		}
	}

	ContinuousTimeline(TimeRange range, T defaultValue, std::initializer_list<Timed<T>> initializerList) :
		ContinuousTimeline(range, defaultValue, initializerList.begin(), initializerList.end())
	{}

	using BoundedTimeline<T>::clear;

	void clear(const TimeRange& range) override {
		set(Timed<T>(range, defaultValue));
	}

private:
	T defaultValue;
};
