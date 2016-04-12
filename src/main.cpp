#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/optional.hpp>
#include <format.h>
#include <tclap/CmdLine.h>
#include "audio/WaveFileReader.h"
#include "phoneExtraction.h"
#include "mouthAnimation.h"
#include "appInfo.h"
#include "NiceCmdLineOutput.h"
#include "ProgressBar.h"
#include "logging.h"
#include <gsl_util.h>
#include <tools.h>
#include <Timeline.h>

using std::exception;
using std::string;
using std::vector;
using std::unique_ptr;
using std::map;
using std::chrono::duration;
using std::chrono::duration_cast;
using boost::filesystem::path;
using boost::property_tree::ptree;

namespace tclap = TCLAP;

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
		return std::make_unique<WaveFileReader>(filePath);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(fmt::format("Could not open sound file {0}.", filePath)));
	}
}

ptree createXmlTree(const path& filePath, const Timeline<Phone>& phones, const Timeline<Shape>& shapes) {
	ptree tree;

	// Add sound file path
	tree.add("rhubarbResult.info.soundFile", filePath.string());

	// Add phones
	for (auto& timedPhone : phones) {
		ptree& phoneElement = tree.add("rhubarbResult.phones.phone", timedPhone.getValue());
		phoneElement.add("<xmlattr>.start", formatDuration(timedPhone.getStart()));
		phoneElement.add("<xmlattr>.duration", formatDuration(timedPhone.getLength()));
	}

	// Add mouth cues
	for (auto& timedShape : shapes) {
		ptree& mouthCueElement = tree.add("rhubarbResult.mouthCues.mouthCue", timedShape.getValue());
		mouthCueElement.add("<xmlattr>.start", formatDuration(timedShape.getStart()));
		mouthCueElement.add("<xmlattr>.duration", formatDuration(timedShape.getLength()));
	}

	return tree;
}

// Tell TCLAP how to handle our types
namespace TCLAP {
	template<>
	struct ArgTraits<LogLevel> {
		typedef ValueLike ValueCategory;
	};
}

int main(int argc, char *argv[]) {
	auto pausableStderrSink = addPausableStderrSink(LogLevel::Warning);
	pausableStderrSink->pause();

	// Define command-line parameters
	const char argumentValueSeparator = ' ';
	tclap::CmdLine cmd(appName, argumentValueSeparator, appVersion);
	cmd.setExceptionHandling(false);
	cmd.setOutput(new NiceCmdLineOutput());
	auto logLevels = vector<LogLevel>(getEnumValues<LogLevel>());
	tclap::ValuesConstraint<LogLevel> logLevelConstraint(logLevels);
	tclap::ValueArg<LogLevel> logLevel("", "logLevel", "The minimum log level to log", false, LogLevel::Debug, &logLevelConstraint, cmd);
	tclap::ValueArg<string> logFileName("", "logFile", "The log file path.", false, string(), "string", cmd);
	tclap::ValueArg<string> dialog("d", "dialog", "The text of the dialog.", false, string(), "string", cmd);
	tclap::UnlabeledValueArg<string> inputFileName("inputFile", "The input file. Must be a sound file in WAVE format.", true, "", "string", cmd);

	try {
		auto resumeLogging = gsl::finally([&]() {
			std::cerr << std::endl << std::endl;
			pausableStderrSink->resume();
			std::cerr << std::endl;
		});

		// Parse command line
		cmd.parse(argc, argv);

		// Set up log file
		if (logFileName.isSet()) {
			addFileSink(path(logFileName.getValue()), logLevel.getValue());
		}

		// Detect phones
		const int columnWidth = 30;
		std::cerr << std::left;
		std::cerr << std::setw(columnWidth) << "Analyzing input file";
		Timeline<Phone> phones{};
		{
			ProgressBar progressBar;
			phones = detectPhones(
				createAudioStream(inputFileName.getValue()),
				dialog.isSet() ? dialog.getValue() : boost::optional<string>(),
				progressBar);
		}
		std::cerr << "Done" << std::endl;

		// Generate mouth shapes
		std::cerr << std::setw(columnWidth) << "Generating mouth shapes";
		Timeline<Shape> shapes = animate(phones);
		std::cerr << "Done" << std::endl;

		std::cerr << std::endl;

		// Print XML
		ptree xmlTree = createXmlTree(inputFileName.getValue(), phones, shapes);
		boost::property_tree::write_xml(std::cout, xmlTree, boost::property_tree::xml_writer_settings<string>(' ', 2));

		return 0;
	} catch (tclap::ArgException& e) {
		// Error parsing command-line args.
		cmd.getOutput()->failure(cmd, e);
		std::cerr << std::endl;
		return 1;
	} catch (tclap::ExitException&) {
		// A built-in TCLAP command (like --help) has finished. Exit application.
		std::cerr << std::endl;
		return 0;
	} catch (const exception& e) {
		// Generic error
		std::cerr << "An error occurred.\n" << getMessage(e) << std::endl;
		return 1;
	}
}
