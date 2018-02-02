#pragma once

#include <tuple>

namespace std {
	
	namespace {

		template <typename T>
		void hash_combine(size_t& seed, const T& value) {
			seed ^= std::hash<T>()(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		// Recursive template code derived from Matthieu M.
		template <typename Tuple, size_t Index = tuple_size<Tuple>::value - 1>
		struct HashValueImpl {
			static void apply(size_t& seed, const Tuple& tuple) {
				HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
				hash_combine(seed, std::get<Index>(tuple));
			}
		};

		template <typename Tuple>
		struct HashValueImpl<Tuple, 0> {
			static void apply(size_t& seed, const Tuple& tuple) {
				hash_combine(seed, std::get<0>(tuple));
			}
		};
	}

	template <typename ... TT>
	struct hash<tuple<TT...>> {
		size_t operator()(const tuple<TT...>& tt) const {
			size_t seed = 0;
			HashValueImpl<tuple<TT...> >::apply(seed, tt);
			return seed;
		}
	};

}