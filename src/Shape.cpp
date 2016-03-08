#include "Shape.h"

#include <tuple>

using std::string;
using std::vector;
using std::tuple;
using std::make_tuple;

template <>
const string& getEnumTypeName<Shape>() {
	static const string name = "Shape";
	return name;
}

template <>
const vector<tuple<Shape, string>>& getEnumMembers<Shape>() {
	static const vector<tuple<Shape, string>> values = {
		make_tuple(Shape::A, "A"),
		make_tuple(Shape::B, "B"),
		make_tuple(Shape::C, "C"),
		make_tuple(Shape::D, "D"),
		make_tuple(Shape::E, "E"),
		make_tuple(Shape::F, "F"),
		make_tuple(Shape::G, "G"),
		make_tuple(Shape::H, "H")
	};
	return values;
}

std::ostream& operator<<(std::ostream& stream, Shape value) {
	return stream << enumToString(value);
}

std::istream& operator>>(std::istream& stream, Shape& value) {
	string name;
	stream >> name;
	value = parseEnum<Shape>(name);
	return stream;
}
