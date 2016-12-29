#include "TsvExporter.h"
#include "targetShapeSet.h"

void TsvExporter::exportAnimation(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& animation, const ShapeSet& targetShapeSet, std::ostream& outputStream) {
	UNUSED(inputFilePath);

	// Output shapes with start times
	for (auto& timedShape : animation) {
		outputStream << formatDuration(timedShape.getStart()) << "\t" << timedShape.getValue() << "\n";
	}

	// Output closed mouth with end time
	outputStream << formatDuration(animation.getRange().getEnd()) << "\t" << convertToTargetShapeSet(Shape::X, targetShapeSet) << "\n";
}
