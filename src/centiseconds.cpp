#include <chrono>
#include <ostream>
#include "Centiseconds.h"

std::ostream& operator <<(std::ostream& stream, const centiseconds cs) {
	return stream << cs.count() << "cs";
}
