#include "XmlExporter.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "exporterTools.h"

using std::string;
using boost::property_tree::ptree;

void XmlExporter::exportShapes(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& shapes, std::ostream& outputStream) {
	ptree tree;

	// Add metadata
	tree.put("rhubarbResult.metadata.soundFile", inputFilePath.string());
	tree.put("rhubarbResult.metadata.duration", formatDuration(shapes.getRange().getDuration()));

	// Add mouth cues
	for (auto& timedShape : dummyShapeIfEmpty(shapes)) {
		ptree& mouthCueElement = tree.add("rhubarbResult.mouthCues.mouthCue", timedShape.getValue());
		mouthCueElement.put("<xmlattr>.start", formatDuration(timedShape.getStart()));
		mouthCueElement.put("<xmlattr>.end", formatDuration(timedShape.getEnd()));
	}

#if BOOST_VERSION < 105600 // Support legacy syntax
	using writer_setting = boost::property_tree::xml_writer_settings<char>;
#else
	using writer_setting = boost::property_tree::xml_writer_settings<string>;
#endif
	write_xml(outputStream, tree, writer_setting(' ', 2));
}
