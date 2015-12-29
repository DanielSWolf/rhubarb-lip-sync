#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <format.h>
#include <tclap/CmdLine.h>
#include "audioInput/WaveFileReader.h"
#include "phoneExtraction.h"
#include "mouthAnimation.h"
#include "platformTools.h"
#include "appInfo.h"

using std::exception;
using std::string;
using std::unique_ptr;
using std::map;
using std::chrono::duration;
using std::chrono::duration_cast;
using boost::filesystem::path;
using boost::property_tree::ptree;

string getMessage(const exception& e) {
	string result(e.what());
	try {
		std::rethrow_if_nested(e);
	} catch(const exception& innerException) {
		result += "\n" + getMessage(innerException);
	} catch(...) {}

	return result;
}

unique_ptr<AudioStream> createAudioStream(path filePath) {
	try {
		return unique_ptr<AudioStream>(new WaveFileReader(filePath));
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Could not open sound file.") );
	}
}

string formatDuration(duration<double> seconds) {
	return fmt::format("{0:.2f}", seconds.count());
}

ptree createXmlTree(const path& filePath, const map<centiseconds, Phone>& phones, const map<centiseconds, Shape>& shapes) {
	ptree tree;

	// Add sound file path
	tree.add("rhubarbResult.info.soundFile", filePath.string());

	// Add phones
	for (auto it = phones.cbegin(), itNext = ++phones.cbegin(); itNext != phones.cend(); ++it, ++itNext) {
		auto pair = *it;
		auto nextPair = *itNext;
		ptree& phoneElement = tree.add("rhubarbResult.phones.phone", pair.second);
		phoneElement.add("<xmlattr>.start", formatDuration(pair.first));
		phoneElement.add("<xmlattr>.duration", formatDuration(nextPair.first - pair.first));
	}

	// Add mouth cues
	for (auto it = shapes.cbegin(), itNext = ++shapes.cbegin(); itNext != shapes.cend(); ++it, ++itNext) {
		auto pair = *it;
		auto nextPair = *itNext;
		ptree& mouthCueElement = tree.add("rhubarbResult.mouthCues.mouthCue", pair.second);
		mouthCueElement.add("<xmlattr>.start", formatDuration(pair.first));
		mouthCueElement.add("<xmlattr>.duration", formatDuration(nextPair.first - pair.first));
	}

	return tree;
}

int main(int argc, char *argv[]) {
	try {
	// Define command-line parameters
	const char argumentValueSeparator = ' ';
	TCLAP::CmdLine cmd(appName, argumentValueSeparator, appVersion);
	cmd.setExceptionHandling(false);
	TCLAP::UnlabeledValueArg<string> inputFileName("inputFile", "The input file. Must be a sound file in WAVE format.", true, "", "string", cmd);

		// Parse command line
		cmd.parse(argc, argv);

		// Create audio streams
		unique_ptr<AudioStream> audioStream = createAudioStream(inputFileName.getValue());

		// Detect phones
		map<centiseconds, Phone> phones = detectPhones(std::move(audioStream));

		// Generate mouth shapes
		map<centiseconds, Shape> shapes = animate(phones);

		// Print XML
		ptree xmlTree = createXmlTree(inputFileName.getValue(), phones, shapes);
		boost::property_tree::write_xml(std::cout, xmlTree, boost::property_tree::xml_writer_settings<string>(' ', 2));

		return 0;
	} catch (const TCLAP::ArgException& e) {
		std::cerr << "Invalid command-line arguments regarding `" << e.argId() << "`. " << e.error();
		return 1;
	} catch (const exception& e) {
		std::cerr << "An error occurred. " << getMessage(e);
		return 1;
	}
}
