#pragma once

#include <map>
#include <string>

#include "core/Shape.h"
#include "Exporter.h"

// Exporter for Moho's switch data file format
class DatExporter : public Exporter {
public:
    DatExporter(const ShapeSet& targetShapeSet, double frameRate, bool convertToPrestonBlair);
    void exportAnimation(const ExporterInput& input, std::ostream& outputStream) override;

private:
    int toFrameNumber(centiseconds time) const;
    std::string toString(Shape shape) const;

    double frameRate;
    bool convertToPrestonBlair;
    std::map<Shape, std::string> prestonBlairShapeNames;
};
