#pragma once
#include <vector>

template<typename TCollection>
std::vector<std::pair<typename TCollection::value_type, typename TCollection::value_type>> getPairs(const TCollection& collection) {
	using TElement = typename TCollection::value_type;
	using TPair = std::pair<TElement, TElement>;

	auto begin = collection.begin();
	auto end = collection.end();
	if (begin == end || ++TCollection::const_iterator(begin) == end) {
		return std::vector<TPair>();
	}

	std::vector<TPair> result;
	for (auto first = begin, second = ++TCollection::const_iterator(begin); second != end; ++first, ++second) {
		result.push_back(TPair(*first, *second));
	}
	return result;
}
