#include "exporterTools.h"

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(const Timeline<Shape>& shapes) {
	std::vector<Timed<Shape>> result;
	std::copy(shapes.begin(), shapes.end(), std::back_inserter(result));
	if (result.empty()) {
		// Add zero-length empty mouth
		result.push_back(Timed<Shape>(0_cs, 0_cs, Shape::X));
	}
	return result;
}
