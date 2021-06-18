#pragma once

#include "core/Shape.h"
#include "time/ContinuousTimeline.h"
#include <filesystem>

class ExporterInput {
public:
	ExporterInput(
		const std::filesystem::path& inputFilePath,
		const JoiningContinuousTimeline<Shape>& animation,
		const ShapeSet& targetShapeSet) :
		inputFilePath(inputFilePath),
		animation(animation),
		targetShapeSet(targetShapeSet) {}

	std::filesystem::path inputFilePath;
	JoiningContinuousTimeline<Shape> animation;
	ShapeSet targetShapeSet;
};

class Exporter {
public:
	virtual ~Exporter() {}
	virtual void exportAnimation(const ExporterInput& input, std::ostream& outputStream) = 0;
};
