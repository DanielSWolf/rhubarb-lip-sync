#include "Shape.h"

using std::string;

ShapeConverter& ShapeConverter::get() {
	static ShapeConverter converter;
	return converter;
}

string ShapeConverter::getTypeName() {
	return "Shape";
}

EnumConverter<Shape>::member_data ShapeConverter::getMemberData() {
	return member_data{
		{ Shape::A, "A" },
		{ Shape::B, "B" },
		{ Shape::C, "C" },
		{ Shape::D, "D" },
		{ Shape::E, "E" },
		{ Shape::F, "F" },
		{ Shape::G, "G" },
		{ Shape::H, "H" }
	};
}

std::ostream& operator<<(std::ostream& stream, Shape value) {
	return ShapeConverter::get().write(stream, value);
}

std::istream& operator>>(std::istream& stream, Shape& value) {
	return ShapeConverter::get().read(stream, value);
}
