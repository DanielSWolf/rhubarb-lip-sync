#include "JsonExporter.h"
#include "exporterTools.h"

using std::string;

string escapeJsonString(const string& s) {
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

void JsonExporter::exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, const ShapeSet& targetShapeSet, std::ostream& outputStream) {
	// Export as JSON.
	// I'm not using a library because the code is short enough without one and it lets me control the formatting.
	outputStream << "{\n";
	outputStream << "  \"metadata\": {\n";
	outputStream << "    \"soundFile\": \"" << escapeJsonString(inputFilePath.string()) << "\",\n";
	outputStream << "    \"duration\": " << formatDuration(shapes.getRange().getDuration()) << "\n";
	outputStream << "  },\n";
	outputStream << "  \"mouthCues\": [\n";
	bool isFirst = true;
	for (auto& timedShape : dummyShapeIfEmpty(shapes, targetShapeSet)) {
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
