#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <format.h>
#include "audio_input/WaveFileReader.h"
#include "phone_extraction.h"
#include "mouth_animation.h"
#include "platform_tools.h"

using std::exception;
using std::string;
using std::wstring;
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
		// Get sound file name
		std::vector<wstring> commandLineArgs = getCommandLineArgs(argc, argv);
		if (commandLineArgs.size() != 2) {
			throw std::runtime_error("Invalid command line arguments. Call with sound file name as sole argument.");
		}
		wstring soundFileName = commandLineArgs[1];

		// Create audio streams
		unique_ptr<AudioStream> audioStream = createAudioStream(soundFileName);

		// Detect phones
		map<centiseconds, Phone> phones = detectPhones(std::move(audioStream));

		// Generate mouth shapes
		map<centiseconds, Shape> shapes = animate(phones);

		// Print XML
		boost::property_tree::ptree xmlTree = createXmlTree(soundFileName, phones, shapes);
		boost::property_tree::write_xml(std::cout, xmlTree, boost::property_tree::xml_writer_settings<string>(' ', 2));

		return 0;
	} catch (const exception& e) {
		std::cerr << "An error occurred. " << getMessage(e);
		return 1;
	}
}
