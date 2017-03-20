#include "XmlExporter.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/version.hpp>
#include "exporterTools.h"

using std::string;
using boost::property_tree::ptree;

void XmlExporter::exportAnimation(const boost::filesystem::path& inputFilePath, const JoiningContinuousTimeline<Shape>& animation, const ShapeSet& targetShapeSet, std::ostream& outputStream) {
	ptree tree;

	// Add metadata
	tree.put("rhubarbResult.metadata.soundFile", inputFilePath.string());
	tree.put("rhubarbResult.metadata.duration", formatDuration(animation.getRange().getDuration()));

	// Add mouth cues
	for (auto& timedShape : dummyShapeIfEmpty(animation, targetShapeSet)) {
		ptree& mouthCueElement = tree.add("rhubarbResult.mouthCues.mouthCue", timedShape.getValue());
		mouthCueElement.put("<xmlattr>.start", formatDuration(timedShape.getStart()));
		mouthCueElement.put("<xmlattr>.end", formatDuration(timedShape.getEnd()));
	}

#ifndef BOOST_VERSION	//present in version.hpp
        #error "Could not detect Boost version."
#endif

#if BOOST_VERSION < 105600 // Support legacy syntax
	using writer_setting = boost::property_tree::xml_writer_settings<char>;
#else
	using writer_setting = boost::property_tree::xml_writer_settings<string>;
#endif
	write_xml(outputStream, tree, writer_setting(' ', 2));
}
