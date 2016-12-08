#pragma once

#include "Exporter.h"

class JsonExporter : public Exporter {
public:
	void exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, std::ostream& outputStream) override;
};
