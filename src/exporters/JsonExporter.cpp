#include "JsonExporter.h"
#include "exporterTools.h"
#include <utf8.h>

using std::string;

string escapeJsonString(const string& s) {
	// JavaScript uses UTF-16 internally. As a result, character escaping in JSON strings is UTF-16-based.
	// Convert string to UTF-16
	std::u16string utf16String;
	utf8::utf8to16(s.begin(), s.end(), std::back_inserter(utf16String));

	string result;
	for (char16_t c : utf16String) {
		switch (c) {
		case '"':  result += "\\\""; break;
		case '\\': result += "\\\\"; break;
		case '\b': result += "\\b"; break;
		case '\f': result += "\\f"; break;
		case '\n': result += "\\n"; break;
		case '\r': result += "\\r"; break;
		case '\t': result += "\\t"; break;
		default:
			{
				bool needsEscaping = c < '\x20' || c >= 0x80;
				if (needsEscaping) {
					result += fmt::format("\\u{0:04x}", c);
				} else {
					result += static_cast<char>(c);
				}
			}
		}
	}
	return result;
}

void JsonExporter::exportAnimation(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& animation, const ShapeSet& targetShapeSet, std::ostream& outputStream) {
	// Export as JSON.
	// I'm not using a library because the code is short enough without one and it lets me control the formatting.
	outputStream << "{\n";
	outputStream << "  \"metadata\": {\n";
	outputStream << "    \"soundFile\": \"" << escapeJsonString(inputFilePath.string()) << "\",\n";
	outputStream << "    \"duration\": " << formatDuration(animation.getRange().getDuration()) << "\n";
	outputStream << "  },\n";
	outputStream << "  \"mouthCues\": [\n";
	bool isFirst = true;
	for (auto& timedShape : dummyShapeIfEmpty(animation, targetShapeSet)) {
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
