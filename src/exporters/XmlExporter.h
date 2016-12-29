#pragma once

#include "Exporter.h"

class XmlExporter : public Exporter {
public:
	void exportAnimation(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& animation, const ShapeSet& targetShapeSet, std::ostream& outputStream) override;
};
