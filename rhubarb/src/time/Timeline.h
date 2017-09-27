#pragma once
#include "Timed.h"
#include <set>
#include <boost/optional.hpp>
#include <type_traits>
#include "tools/tools.h"

enum class FindMode {
	SampleLeft,
	SampleRight,
	SearchLeft,
	SearchRight
};

namespace internal {
	template<typename T>
	bool valueEquals(const Timed<T>& lhs, const Timed<T>& rhs) {
		return lhs.getValue() == rhs.getValue();
	}

	template<>
	inline bool valueEquals<void>(const Timed<void>& lhs, const Timed<void>& rhs) {
		UNUSED(lhs);
		UNUSED(rhs);
		return true;
	}
}

template<typename T, bool AutoJoin = false>
class Timeline {
public:
	using time_type = TimeRange::time_type;

private:
	struct compare {
		bool operator()(const Timed<T>& lhs, const Timed<T>& rhs) const {
			return lhs.getStart() < rhs.getStart();
		}
		bool operator()(const time_type& lhs, const Timed<T>& rhs) const {
			return lhs < rhs.getStart();
		}
		bool operator()(const Timed<T>& lhs, const time_type& rhs) const {
			return lhs.getStart() < rhs;
		}
		using is_transparent = int;
	};

public:
	using set_type = std::set<Timed<T>, compare>;
	using const_iterator = typename set_type::const_iterator;
	using iterator = const_iterator;
	using reverse_iterator = typename set_type::reverse_iterator;
	using size_type = size_t;
	using value_type = Timed<T>;
	using reference = const value_type&;

	class ReferenceWrapper {
	public:
		operator boost::optional<T>() const {
			auto optional = timeline.get(time);
			return optional ? optional->getValue() : boost::optional<T>();
		}

		operator const T&() const {
			auto optional = timeline.get(time);
			assert(optional);
			return optional->getValue();
		}

		ReferenceWrapper& operator=(boost::optional<const T&> value) {
			if (value) {
				timeline.set(time, time + time_type(1), *value);
			} else {
				timeline.clear(time, time + time_type(1));
			}
			return *this;
		}

	private:
		friend class Timeline;

		ReferenceWrapper(Timeline& timeline, time_type time) :
			timeline(timeline),
			time(time)
		{}

		Timeline& timeline;
		time_type time;
	};

	Timeline() {}

	template<typename InputIterator>
	Timeline(InputIterator first, InputIterator last) {
		for (auto it = first; it != last; ++it) {
			// Virtual function call in constructor. Derived constructors don't call this one.
			Timeline::set(*it);
		}
	}

	template<typename collection_type>
	explicit Timeline(collection_type collection) :
		Timeline(collection.begin(), collection.end())
	{}

	explicit Timeline(std::initializer_list<Timed<T>> initializerList) :
		Timeline(initializerList.begin(), initializerList.end())
	{}

	virtual ~Timeline() {}

	bool empty() const {
		return elements.empty();
	}

	size_type size() const {
		return elements.size();
	}

	virtual TimeRange getRange() const {
		return empty()
			? TimeRange(time_type::zero(), time_type::zero())
			: TimeRange(begin()->getStart(), rbegin()->getEnd());
	}

	iterator begin() const {
		return elements.begin();
	}

	iterator end() const {
		return elements.end();
	}

	reverse_iterator rbegin() const {
		return elements.rbegin();
	}

	reverse_iterator rend() const {
		return elements.rend();
	}

	iterator find(time_type time, FindMode findMode = FindMode::SampleRight) const {
		switch (findMode) {
		case FindMode::SampleLeft: {
			iterator left = find(time, FindMode::SearchLeft);
			return left != end() && left->getEnd() >= time ? left : end();
		}
		case FindMode::SampleRight: {
			iterator right = find(time, FindMode::SearchRight);
			return right != end() && right->getStart() <= time ? right : end();
		}
		case FindMode::SearchLeft: {
			// Get first element starting >= time
			iterator it = elements.lower_bound(time);

			// Go one element back
			return it != begin() ? --it : end();
		}
		case FindMode::SearchRight: {
			// Get first element starting > time
			iterator it = elements.upper_bound(time);

			// Go one element back
			if (it != begin()) {
				iterator left = it;
				--left;
				if (left->getEnd() > time) return left;
			}
			return it;
		}
		default:
			throw std::invalid_argument("Unexpected find mode.");
		}
	}

	boost::optional<const Timed<T>&> get(time_type time) const {
		iterator it = find(time);
		return (it != end()) ? *it : boost::optional<const Timed<T>&>();
	}

	virtual void clear(const TimeRange& range) {
		// Make sure the time range is not empty
		if (range.empty()) return;

		// Split overlapping elements
		splitAt(range.getStart());
		splitAt(range.getEnd());

		// Erase overlapping elements
		elements.erase(find(range.getStart(), FindMode::SearchRight), find(range.getEnd(), FindMode::SearchRight));
	}

	void clear(time_type start, time_type end) {
		clear(TimeRange(start, end));
	}

	virtual iterator set(Timed<T> timedValue) {
		// Make sure the timed value is not empty
		if (timedValue.getTimeRange().empty()) {
			return end();
		}

		if (AutoJoin) {
			// Extend the timed value if it touches elements with equal value
			iterator elementBefore = find(timedValue.getStart(), FindMode::SampleLeft);
			if (elementBefore != end() && ::internal::valueEquals(*elementBefore, timedValue)) {
				timedValue.getTimeRange().resize(elementBefore->getStart(), timedValue.getEnd());
			}
			iterator elementAfter = find(timedValue.getEnd(), FindMode::SampleRight);
			if (elementAfter != end() && ::internal::valueEquals(*elementAfter, timedValue)) {
				timedValue.getTimeRange().resize(timedValue.getStart(), elementAfter->getEnd());
			}
		}

		// Erase overlapping elements
		Timeline::clear(timedValue.getTimeRange());

		// Add timed value
		return elements.insert(timedValue).first;
	}

	template<typename TElement = T>
	iterator set(const TimeRange& timeRange, const std::enable_if_t<!std::is_void<TElement>::value, T>& value) {
		return set(Timed<T>(timeRange, value));
	}

	template<typename TElement = T>
	iterator set(time_type start, time_type end, const std::enable_if_t<!std::is_void<TElement>::value, T>& value) {
		return set(Timed<T>(start, end, value));
	}

	template<typename TElement = T>
	std::enable_if_t<std::is_void<TElement>::value, iterator>
	set(time_type start, time_type end) {
		return set(Timed<void>(start, end));
	}

	ReferenceWrapper operator[](time_type time) {
		return ReferenceWrapper(*this, time);
	}

	// ReSharper disable once CppConstValueFunctionReturnType
	const ReferenceWrapper operator[](time_type time) const {
		return ReferenceWrapper(*this, time);
	}

	// Combines adjacent equal elements into one
	template<bool autoJoin = AutoJoin, typename = std::enable_if_t<!autoJoin>>
	void joinAdjacent() {
		Timeline copy(*this);
		for (auto it = copy.begin(); it != copy.end(); ++it) {
			const auto rangeBegin = it;
			auto rangeEnd = std::next(rangeBegin);
			while (rangeEnd != copy.end() && rangeEnd->getStart() == rangeBegin->getEnd() && ::internal::valueEquals(*rangeEnd, *rangeBegin)) {
				++rangeEnd;
			}

			if (rangeEnd != std::next(rangeBegin)) {
				Timed<T> combined = *rangeBegin;
				combined.setTimeRange({rangeBegin->getStart(), rangeEnd->getEnd()});
				set(combined);
				it = rangeEnd;
			}
		}
	}

	virtual void shift(time_type offset) {
		if (offset == time_type::zero()) return;

		set_type newElements;
		for (Timed<T> element : elements) {
			element.getTimeRange().shift(offset);
			newElements.insert(element);
		}
		elements = std::move(newElements);
	}

	Timeline(const Timeline&) = default;
	Timeline(Timeline&&) = default;
	Timeline& operator=(const Timeline&) = default;
	Timeline& operator=(Timeline&&) = default;

	bool operator==(const Timeline& rhs) const {
		return equals(rhs);
	}

	bool operator!=(const Timeline& rhs) const {
		return !equals(rhs);
	}

protected:
	bool equals(const Timeline& rhs) const {
		return elements == rhs.elements;
	}

private:
	void splitAt(time_type splitTime) {
		iterator elementBefore = find(splitTime - time_type(1));
		iterator elementAfter = find(splitTime);
		if (elementBefore != elementAfter || elementBefore == end()) return;
		
		Timed<T> first = *elementBefore;
		Timed<T> second = *elementBefore;
		elements.erase(elementBefore);
		first.getTimeRange().resize(first.getStart(), splitTime);
		elements.insert(first);
		second.getTimeRange().resize(splitTime, second.getEnd());
		elements.insert(second);
	}

	set_type elements;
};

template<typename T>
using JoiningTimeline = Timeline<T, true>;

template<typename T, bool AutoJoin>
std::ostream& operator<<(std::ostream& stream, const Timeline<T, AutoJoin>& timeline) {
	stream << "Timeline{";
	bool isFirst = true;
	for (auto element : timeline) {
		if (!isFirst) stream << ", ";
		isFirst = false;
		stream << element;
	}
	return stream << "}";
}
