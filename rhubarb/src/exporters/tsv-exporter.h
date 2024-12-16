#pragma once

#include "exporter.h"

class TsvExporter : public Exporter {
public:
    void exportAnimation(const ExporterInput& input, std::ostream& outputStream) override;
};
