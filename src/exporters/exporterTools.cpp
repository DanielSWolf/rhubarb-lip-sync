#include "exporterTools.h"
#include "animation/targetShapeSet.h"

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(const JoiningTimeline<Shape>& animation, const ShapeSet& targetShapeSet) {
	std::vector<Timed<Shape>> result;
	std::copy(animation.begin(), animation.end(), std::back_inserter(result));
	if (result.empty()) {
		// Add zero-length empty mouth
		result.push_back(Timed<Shape>(0_cs, 0_cs, convertToTargetShapeSet(Shape::X, targetShapeSet)));
	}
	return result;
}
