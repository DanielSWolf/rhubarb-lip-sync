#include "TsvExporter.h"
#include "animation/targetShapeSet.h"

void TsvExporter::exportAnimation(const ExporterInput& input, std::ostream& outputStream) {
	// Output shapes with start times
	for (auto& timedShape : input.animation) {
		outputStream << formatDuration(timedShape.getStart()) << "\t" << timedShape.getValue() << "\n";
	}

	// Output closed mouth with end time
	outputStream
		<< formatDuration(input.animation.getRange().getEnd())
		<< "\t"
		<< convertToTargetShapeSet(Shape::X, input.targetShapeSet)
		<< "\n";
}
