#pragma once

#include <Shape.h>
#include "ContinuousTimeline.h"
#include <boost/filesystem/path.hpp>

class Exporter {
public:
	virtual ~Exporter() {}
	virtual void exportAnimation(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& animation, const ShapeSet& targetShapeSet, std::ostream& outputStream) = 0;
};
