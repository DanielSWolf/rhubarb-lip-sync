#include "ExportFormat.h"

using std::string;

ExportFormatConverter& ExportFormatConverter::get() {
	static ExportFormatConverter converter;
	return converter;
}

string ExportFormatConverter::getTypeName() {
	return "ExportFormat";
}

EnumConverter<ExportFormat>::member_data ExportFormatConverter::getMemberData() {
	return member_data{
		{ ExportFormat::Tsv,		"tsv" },
		{ ExportFormat::Xml,		"xml" },
		{ ExportFormat::Json,		"json" }
	};
}

std::ostream& operator<<(std::ostream& stream, ExportFormat value) {
	return ExportFormatConverter::get().write(stream, value);
}

std::istream& operator>>(std::istream& stream, ExportFormat& value) {
	return ExportFormatConverter::get().read(stream, value);
}
