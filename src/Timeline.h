#pragma once
#include "Timed.h"
#include <set>
#include <algorithm>

template<typename T>
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
		using is_transparent = int;
	};

public:
	using set_type = std::set<Timed<T>, compare>;
	using const_iterator = typename set_type::const_iterator;
	using iterator = const_iterator;
	using reverse_iterator = typename set_type::reverse_iterator;
	using size_type = size_t;
	using value_type = Timed<T>;

	class reference {
	public:
		using const_reference = const T&;

		operator const_reference() const {
			return timeline.get(time).getValue();
		}

		reference& operator=(const T& value) {
			timeline.set(time, time + time_type(1), value);
			return *this;
		}

	private:
		friend class Timeline;

		reference(Timeline& timeline, time_type time) :
			timeline(timeline),
			time(time)
		{}

		Timeline& timeline;
		time_type time;
	};

	explicit Timeline(const Timed<T> timedValue) :
		elements(),
		range(timedValue)
	{
		if (timedValue.getLength() != time_type::zero()) {
			elements.insert(timedValue);
		}
	};

	explicit Timeline(const TimeRange& timeRange, const T& value = T()) :
		Timeline(Timed<T>(timeRange, value))
	{ }

	Timeline(time_type start, time_type end, const T& value = T()) :
		Timeline(Timed<T>(start, end, value))
	{}

	template<typename InputIterator>
	Timeline(InputIterator first, InputIterator last, const T& value = T()) :
		Timeline(getRange(first, last), value)
	{
		for (auto it = first; it != last; ++it) {
			set(*it);
		}
	}

	explicit Timeline(std::initializer_list<Timed<T>> initializerList, const T& value = T()) :
		Timeline(initializerList.begin(), initializerList.end(), value)
	{}

	bool empty() const {
		return elements.empty();
	}

	size_type size() const {
		return elements.size();
	}

	const TimeRange& getRange() const {
		return range;
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

	iterator find(time_type time) const {
		if (time < range.getStart() || time >= range.getEnd()) {
			return elements.end();
		}

		iterator it = elements.upper_bound(time);
		--it;
		return it;
	}

	const Timed<T>& get(time_type time) const {
		iterator it = find(time);
		if (it == elements.end()) {
			throw std::invalid_argument("Argument out of range.");
		}
		return *it;
	}

	iterator set(Timed<T> timedValue) {
		// Make sure the timed value overlaps with our range
		if (timedValue.getEnd() <= range.getStart() || timedValue.getStart() >= range.getEnd()) {
			return elements.end();
		}

		// Make sure the timed value is not empty
		if (timedValue.getLength() == time_type::zero()) {
			return elements.end();
		}

		// Trim the timed value to our range
		timedValue.resize(
			std::max(timedValue.getStart(), range.getStart()),
			std::min(timedValue.getEnd(), range.getEnd()));

		// Extend the timed value if it touches elements with equal value
		bool isFlushLeft = timedValue.getStart() == range.getStart();
		if (!isFlushLeft) {
			iterator elementBefore = find(timedValue.getStart() - time_type(1));
			if (elementBefore->getValue() == timedValue.getValue()) {
				timedValue.resize(elementBefore->getStart(), timedValue.getEnd());
			}
		}
		bool isFlushRight = timedValue.getEnd() == range.getEnd();
		if (!isFlushRight) {
			iterator elementAfter = find(timedValue.getEnd());
			if (elementAfter->getValue() == timedValue.getValue()) {
				timedValue.resize(timedValue.getStart(), elementAfter->getEnd());
			}
		}

		// Split overlapping elements
		splitAt(timedValue.getStart());
		splitAt(timedValue.getEnd());

		// Erase overlapping elements
		elements.erase(find(timedValue.getStart()), find(timedValue.getEnd()));

		// Add timed value
		return elements.insert(timedValue).first;
	}

	iterator set(const TimeRange& timeRange, const T& value) {
		return set(Timed<T>(timeRange, value));
	}

	iterator set(time_type start, time_type end, const T& value) {
		return set(Timed<T>(start, end, value));
	}

	reference operator[](time_type time) {
		if (time < range.getStart() || time >= range.getEnd()) {
			throw std::invalid_argument("Argument out of range.");
		}
		return reference(*this, time);
	}

	// ReSharper disable once CppConstValueFunctionReturnType
	const reference operator[](time_type time) const {
		return reference(*this, time);
	}

	Timeline(const Timeline&) = default;
	Timeline(Timeline&&) = default;
	Timeline& operator=(const Timeline&) = default;
	Timeline& operator=(Timeline&&) = default;

	bool operator==(const Timeline& rhs) const {
		return range == rhs.range && elements == rhs.elements;
	}

	bool operator!=(const Timeline& rhs) const {
		return !operator==(rhs);
	}

private:
	template<typename InputIterator>
	static TimeRange getRange(InputIterator first, InputIterator last) {
		if (first == last) {
			return TimeRange(time_type::zero(), time_type::zero());
		}

		time_type start = time_type::max();
		time_type end = time_type::min();
		for (auto it = first; it != last; ++it) {
			start = std::min(start, it->getStart());
			end = std::max(end, it->getEnd());
		}
		return TimeRange(start, end);
	}

	void splitAt(time_type splitTime) {
		if (splitTime == range.getStart() || splitTime == range.getEnd()) return;

		iterator elementBefore = find(splitTime - time_type(1));
		iterator elementAfter = find(splitTime);
		if (elementBefore != elementAfter) return;
		
		Timed<T> tmp = *elementBefore;
		elements.erase(elementBefore);
		elements.insert(Timed<T>(tmp.getStart(), splitTime, tmp.getValue()));
		elements.insert(Timed<T>(splitTime, tmp.getEnd(), tmp.getValue()));
	}

	set_type elements;
	TimeRange range;
};

template<typename T>
std::ostream& operator<<(std::ostream& stream, const Timeline<T>& timeline) {
	stream << "Timeline{";
	bool isFirst = true;
	for (auto element : timeline) {
		if (!isFirst) stream << ", ";
		isFirst = false;
		stream << element;
	}
	return stream << "}";
}
