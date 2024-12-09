#include "DatExporter.h"

#include <boost/lexical_cast.hpp>

#include "animation/targetShapeSet.h"

using std::string;
using std::chrono::duration;
using std::chrono::duration_cast;

DatExporter::DatExporter(
    const ShapeSet& targetShapeSet, double frameRate, bool convertToPrestonBlair
) :
    frameRate(frameRate),
    convertToPrestonBlair(convertToPrestonBlair),
    prestonBlairShapeNames{
        {Shape::A, "MBP"},
        {Shape::B, "etc"},
        {Shape::C, "E"},
        {Shape::D, "AI"},
        {Shape::E, "O"},
        {Shape::F, "U"},
        {Shape::G, "FV"},
        {Shape::H, "L"},
        {Shape::X, "rest"},
    } {
    // Animation works with a fixed frame rate of 100.
    // Downsampling to much less than 25 fps may result in dropped frames.
    // Upsampling to more than 100 fps doesn't make sense.
    const double minFrameRate = 24.0;
    const double maxFrameRate = 100.0;

    if (frameRate < minFrameRate || frameRate > maxFrameRate) {
        throw std::runtime_error(
            fmt::format("Frame rate must be between {} and {} fps.", minFrameRate, maxFrameRate)
        );
    }

    if (convertToPrestonBlair) {
        for (Shape shape : targetShapeSet) {
            if (prestonBlairShapeNames.find(shape) == prestonBlairShapeNames.end()) {
                throw std::runtime_error(
                    fmt::format("Mouth shape {} cannot be converted to Preston Blair shape names.")
                );
            }
        }
    }
}

void DatExporter::exportAnimation(const ExporterInput& input, std::ostream& outputStream) {
    outputStream << "MohoSwitch1" << "\n";

    // Output shapes with start times
    int lastFrameNumber = 0;
    for (auto& timedShape : input.animation) {
        const int frameNumber = toFrameNumber(timedShape.getStart());
        if (frameNumber == lastFrameNumber) continue;

        const string shapeName = toString(timedShape.getValue());
        outputStream << frameNumber << " " << shapeName << "\n";
        lastFrameNumber = frameNumber;
    }

    // Output closed mouth with end time
    int frameNumber = toFrameNumber(input.animation.getRange().getEnd());
    if (frameNumber == lastFrameNumber) ++frameNumber;
    const string shapeName = toString(convertToTargetShapeSet(Shape::X, input.targetShapeSet));
    outputStream << frameNumber << " " << shapeName << "\n";
}

string DatExporter::toString(Shape shape) const {
    return convertToPrestonBlair ? prestonBlairShapeNames.at(shape)
                                 : boost::lexical_cast<std::string>(shape);
}

int DatExporter::toFrameNumber(centiseconds time) const {
    return 1 + static_cast<int>(frameRate * duration_cast<duration<double>>(time).count());
}
