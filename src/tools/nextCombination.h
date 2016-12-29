#pragma once

#include <algorithm>

// next_combination Template
// Originally written by Thomas Draper
//
// Designed after next_permutation in STL
// Inspired by Mark Nelson's article http://www.dogma.net/markn/articles/Permutations/
// 
// Start with a sorted container with thee iterators -- first, k, last
// After each iteration, the first k elements of the container will be 
// a combination. When there are no more combinations, the container
// will return to the original sorted order.
template <typename Iterator>
inline bool next_combination(const Iterator first, Iterator k, const Iterator last) {
	// Handle degenerate cases
	if (first == last || std::next(first) == last || first == k || k == last) {
		return false;
	}

	Iterator it1 = k;
	Iterator it2 = std::prev(last);
	// Search down to find first comb entry less than final entry
	while (it1 != first) {
		--it1;
		if (*it1 < *it2) {
			Iterator j = k;
			while (!(*it1 < *j)) {
				++j;
			}
			std::iter_swap(it1, j);
			++it1;
			++j;
			it2 = k;
			std::rotate(it1, j, last);
			while (last != j) {
				++j;
				++it2;
			}
			std::rotate(k, it2, last);
			return true;
		}
	}
	std::rotate(first, k, last);
	return false;
}
