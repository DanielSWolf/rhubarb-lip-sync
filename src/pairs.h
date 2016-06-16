#pragma once
#include <vector>

template<typename TCollection>
std::vector<std::pair<typename TCollection::value_type, typename TCollection::value_type>> getPairs(const TCollection& collection) {
	using TElement = typename TCollection::value_type;
	using TPair = std::pair<TElement, TElement>;
	using TIterator = typename TCollection::const_iterator;

	auto begin = collection.begin();
	auto end = collection.end();
	if (begin == end || ++TIterator(begin) == end) {
		return std::vector<TPair>();
	}

	std::vector<TPair> result;
	for (auto first = begin, second = ++TIterator(begin); second != end; ++first, ++second) {
		result.push_back(TPair(*first, *second));
	}
	return result;
}
