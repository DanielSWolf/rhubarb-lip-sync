#include "Exporter.h"
#include <logging.h>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <tools.h>

using std::string;
using boost::property_tree::ptree;
using std::vector;
using std::tuple;
using std::make_tuple;

template <>
const string& getEnumTypeName<ExportFormat>() {
	static const string name = "ExportFormat";
	return name;
}

template <>
const vector<tuple<ExportFormat, string>>& getEnumMembers<ExportFormat>() {
	static const vector<tuple<ExportFormat, string>> values = {
		make_tuple(ExportFormat::TSV,		"TSV"),
		make_tuple(ExportFormat::XML,		"XML"),
		make_tuple(ExportFormat::JSON,		"JSON")
	};
	return values;
}

std::ostream& operator<<(std::ostream& stream, ExportFormat value) {
	return stream << enumToString(value);
}

std::istream& operator>>(std::istream& stream, ExportFormat& value) {
	string name;
	stream >> name;
	value = parseEnum<ExportFormat>(name);
	return stream;
}

// Makes sure there is at least one mouth shape
std::vector<Timed<Shape>> dummyShapeIfEmpty(const Timeline<Shape>& shapes) {
	std::vector<Timed<Shape>> result;
	std::copy(shapes.begin(), shapes.end(), std::back_inserter(result));
	if (result.empty()) {
		// Add zero-length empty mouth
		result.push_back(Timed<Shape>(centiseconds(0), centiseconds(0), Shape::A));
	}
	return result;
}

void TSVExporter::exportShapes(const boost::filesystem::path& inputFilePath, const Timeline<Shape>& shapes, std::ostream& outputStream) {
	UNUSED(inputFilePath);

	// Output shapes with start times
	for (auto& timedShape : shapes) {
		outputStream << formatDuration(timedShape.getStart()) << "\t" << timedShape.getValue() << "\n";
	}

	// Output closed mouth with end time
	outputStream << formatDuration(shapes.getRange().getEnd()) << "\t" << Shape::A << "\n";
}

void XMLExporter::exportShapes(const boost::filesystem::path& inputFilePath, const Timeline<Shape>& shapes, std::ostream& outputStream) {
	ptree tree;

	// Add metadata
	tree.put("rhubarbResult.metadata.soundFile", inputFilePath.string());
	tree.put("rhubarbResult.metadata.duration", formatDuration(shapes.getRange().getLength()));

	// Add mouth cues
	for (auto& timedShape : dummyShapeIfEmpty(shapes)) {
		ptree& mouthCueElement = tree.add("rhubarbResult.mouthCues.mouthCue", timedShape.getValue());
		mouthCueElement.put("<xmlattr>.start", formatDuration(timedShape.getStart()));
		mouthCueElement.put("<xmlattr>.end", formatDuration(timedShape.getEnd()));
	}

	write_xml(outputStream, tree, boost::property_tree::xml_writer_settings<string>(' ', 2));
}

string escapeJSONString(const string& s) {
	string result;
	for (char c : s) {
		switch (c) {
		case '"':  result += "\\\""; break;
		case '\\': result += "\\\\"; break;
		case '\b': result += "\\b"; break;
		case '\f': result += "\\f"; break;
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\t': result += "\\t"; break;
		default:
			if (c <= '\x1f') {
				result += fmt::format("\\u{0:04x}", c);
			} else {
				result += c;
			}
		}
	}
	return result;
}

void JSONExporter::exportShapes(const boost::filesystem::path& inputFilePath, const Timeline<Shape>& shapes, std::ostream& outputStream) {
	// Export as JSON.
	// I'm not using a library because the code is short enough without one and it lets me control the formatting.
	outputStream << "{\n";
	outputStream << "  \"metadata\": {\n";
	outputStream << "    \"soundFile\": \"" << escapeJSONString(inputFilePath.string()) << "\",\n";
	outputStream << "    \"duration\": " << formatDuration(shapes.getRange().getLength()) << "\n";
	outputStream << "  },\n";
	outputStream << "  \"mouthCues\": [\n";
	bool isFirst = true;
	for (auto& timedShape : dummyShapeIfEmpty(shapes)) {
		if (!isFirst) outputStream << ",\n";
		isFirst = false;
		outputStream << "    { \"start\": " << formatDuration(timedShape.getStart())
			<< ", \"end\": " << formatDuration(timedShape.getEnd())
			<< ", \"value\": \"" << timedShape.getValue() << "\" }";
	}
	outputStream << "\n";
	outputStream << "  ]\n";
	outputStream << "}\n";
}
