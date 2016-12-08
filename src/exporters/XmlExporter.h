#pragma once

#include "Exporter.h"

class XmlExporter : public Exporter {
public:
	void exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, std::ostream& outputStream) override;
};
