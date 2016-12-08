#pragma once

#include <Shape.h>
#include "ContinuousTimeline.h"
#include <boost/filesystem/path.hpp>

class Exporter {
public:
	virtual ~Exporter() {}
	virtual void exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, std::ostream& outputStream) = 0;
};
