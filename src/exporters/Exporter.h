#pragma once

#include <Shape.h>
#include "ContinuousTimeline.h"
#include <boost/filesystem/path.hpp>

class Exporter {
public:
	virtual ~Exporter() {}
	virtual void exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, const ShapeSet& targetShapeSet, std::ostream& outputStream) = 0;
};
