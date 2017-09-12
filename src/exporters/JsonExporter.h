#pragma once

#include "Exporter.h"

class JsonExporter : public Exporter {
public:
	void exportAnimation(const ExporterInput& input, std::ostream& outputStream) override;
};
