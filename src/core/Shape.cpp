#include "Shape.h"

using std::string;
using std::set;

ShapeConverter& ShapeConverter::get() {
	static ShapeConverter converter;
	return converter;
}

set<Shape> ShapeConverter::getBasicShapes() {
	static const set<Shape> result = [] {
		set<Shape> result;
		for (int i = 0; i <= static_cast<int>(Shape::LastBasicShape); ++i) {
			result.insert(static_cast<Shape>(i));
		}
		return result;
	}();
	return result;
}

set<Shape> ShapeConverter::getExtendedShapes() {
	static const set<Shape> result = [] {
		set<Shape> result;
		for (int i = static_cast<int>(Shape::LastBasicShape) + 1; i < static_cast<int>(Shape::EndSentinel); ++i) {
			result.insert(static_cast<Shape>(i));
		}
		return result;
	}();
	return result;
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
		{ Shape::H, "H" },
		{ Shape::X, "X" }
	};
}

std::ostream& operator<<(std::ostream& stream, Shape value) {
	return ShapeConverter::get().write(stream, value);
}

std::istream& operator>>(std::istream& stream, Shape& value) {
	return ShapeConverter::get().read(stream, value);
}
