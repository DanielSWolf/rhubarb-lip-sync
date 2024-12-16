#pragma once

#include "exporter.h"

class JsonExporter : public Exporter {
public:
    void exportAnimation(const ExporterInput& input, std::ostream& outputStream) override;
};
