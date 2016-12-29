#pragma once

#include "Exporter.h"

class JsonExporter : public Exporter {
public:
	void exportAnimation(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& animation, const ShapeSet& targetShapeSet, std::ostream& outputStream) override;
};
