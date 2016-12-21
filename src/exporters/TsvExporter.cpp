#include "TsvExporter.h"
#include "targetShapeSet.h"

void TsvExporter::exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, const ShapeSet& targetShapeSet, std::ostream& outputStream) {
	UNUSED(inputFilePath);

	// Output shapes with start times
	for (auto& timedShape : shapes) {
		outputStream << formatDuration(timedShape.getStart()) << "\t" << timedShape.getValue() << "\n";
	}

	// Output closed mouth with end time
	outputStream << formatDuration(shapes.getRange().getEnd()) << "\t" << convertToTargetShapeSet(Shape::X, targetShapeSet) << "\n";
}
