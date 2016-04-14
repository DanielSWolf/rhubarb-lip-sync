#pragma once

#include <Shape.h>
#include <Timeline.h>
#include <boost/filesystem/path.hpp>
#include <iostream>

enum class ExportFormat {
	TSV,
	XML,
	JSON
};

class ExportFormatConverter : public EnumConverter<ExportFormat> {
public:
	static ExportFormatConverter& get();
protected:
	std::string getTypeName() override;
	member_data getMemberData() override;
};

std::ostream& operator<<(std::ostream& stream, ExportFormat value);

std::istream& operator>>(std::istream& stream, ExportFormat& value);

class Exporter {
public:
	virtual ~Exporter() {}
	virtual void exportShapes(const boost::filesystem::path& inputFilePath, const Timeline<Shape>& shapes, std::ostream& outputStream) = 0;
};

class TSVExporter : public Exporter {
public:
	void exportShapes(const boost::filesystem::path& inputFilePath, const Timeline<Shape>& shapes, std::ostream& outputStream) override;
};

class XMLExporter : public Exporter {
public:
	void exportShapes(const boost::filesystem::path& inputFilePath, const Timeline<Shape>& shapes, std::ostream& outputStream) override;
};

class JSONExporter : public Exporter {
public:
	void exportShapes(const boost::filesystem::path& inputFilePath, const Timeline<Shape>& shapes, std::ostream& outputStream) override;
};
