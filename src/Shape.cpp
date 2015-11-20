#include "Shape.h"

std::string shapeToString(Shape shape) {
	char c = 'A' + static_cast<int>(shape);
	return std::string(&c, 1);
}

std::ostream &operator<<(std::ostream &stream, const Shape shape) {
	return stream << shapeToString(shape);
}
