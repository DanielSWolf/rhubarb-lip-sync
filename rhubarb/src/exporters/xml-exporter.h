#pragma once

#include "exporter.h"

class XmlExporter : public Exporter {
public:
    void exportAnimation(const ExporterInput& input, std::ostream& outputStream) override;
};
