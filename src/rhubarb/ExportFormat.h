#pragma once

#include "tools/EnumConverter.h"

enum class ExportFormat {
	Tsv,
	Xml,
	Json
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
