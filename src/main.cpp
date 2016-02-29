#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/optional.hpp>
#include <format.h>
#include <tclap/CmdLine.h>
#include "audioInput/WaveFileReader.h"
#include "phoneExtraction.h"
#include "mouthAnimation.h"
#include "appInfo.h"
#include "NiceCmdLineOutput.h"
#include "ProgressBar.h"
#include "logging.h"
#include <gsl_util.h>

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

// Tell TCLAP how to handle boost::optional
namespace TCLAP {
	template<>
	struct ArgTraits<boost::optional<string>> {
		typedef TCLAP::StringLike ValueCategory;
	};
}

int main(int argc, char *argv[]) {
	auto logOutputController = initLogging();
	logOutputController->pause();

	// Define command-line parameters
	const char argumentValueSeparator = ' ';
	TCLAP::CmdLine cmd(appName, argumentValueSeparator, appVersion);
	cmd.setExceptionHandling(false);
	cmd.setOutput(new NiceCmdLineOutput());
	TCLAP::UnlabeledValueArg<string> inputFileName("inputFile", "The input file. Must be a sound file in WAVE format.", true, "", "string", cmd);
	TCLAP::ValueArg<boost::optional<string>> dialog("d", "dialog", "The text of the dialog.", false, boost::optional<string>(), "string", cmd);

	try {
		auto resumeLogging = gsl::finally([&]() {
			std::cerr << std::endl << std::endl;
			logOutputController->resume();
			std::cerr << std::endl;
		});

		// Parse command line
		cmd.parse(argc, argv);

		// Detect phones
		const int columnWidth = 30;
		std::cerr << std::left;
		std::cerr << std::setw(columnWidth) << "Analyzing input file";
		map<centiseconds, Phone> phones;
		{
			ProgressBar progressBar;
			phones = detectPhones(
				[&inputFileName]() { return createAudioStream(inputFileName.getValue()); },
				dialog.getValue(),
				progressBar);
		}
		std::cerr << "Done" << std::endl;

		// Generate mouth shapes
		std::cerr << std::setw(columnWidth) << "Generating mouth shapes";
		map<centiseconds, Shape> shapes = animate(phones);
		std::cerr << "Done" << std::endl;

		std::cerr << std::endl;

		// Print XML
		ptree xmlTree = createXmlTree(inputFileName.getValue(), phones, shapes);
		boost::property_tree::write_xml(std::cout, xmlTree, boost::property_tree::xml_writer_settings<string>(' ', 2));

		return 0;
	} catch (TCLAP::ArgException& e) {
		// Error parsing command-line args.
		cmd.getOutput()->failure(cmd, e);
		std::cerr << std::endl;
		return 1;
	} catch (TCLAP::ExitException&) {
		// A built-in TCLAP command (like --help) has finished. Exit application.
		std::cerr << std::endl;
		return 0;
	} catch (const exception& e) {
		// Generic error
		std::cerr << "An error occurred. " << getMessage(e) << std::endl;
		return 1;
	}
}
