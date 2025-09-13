#include "centiseconds.h"

#include <chrono>
#include <ostream>

namespace std {

std::ostream& operator<<(std::ostream& stream, const centiseconds cs) {
    return stream << cs.count() << "cs";
}

} // namespace std
