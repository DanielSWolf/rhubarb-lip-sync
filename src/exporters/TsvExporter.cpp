#include "TsvExporter.h"

void TsvExporter::exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, std::ostream& outputStream) {
	UNUSED(inputFilePath);

	// Output shapes with start times
	for (auto& timedShape : shapes) {
		outputStream << formatDuration(timedShape.getStart()) << "\t" << timedShape.getValue() << "\n";
	}

	// Output closed mouth with end time
	outputStream << formatDuration(shapes.getRange().getEnd()) << "\t" << Shape::X << "\n";
}
