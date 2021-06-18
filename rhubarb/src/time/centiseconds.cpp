#include <chrono>
#include <ostream>
#include "centiseconds.h"

namespace std {

	std::ostream& operator <<(std::ostream& stream, const centiseconds cs) {
		return stream << cs.count() << "cs";
	}

}