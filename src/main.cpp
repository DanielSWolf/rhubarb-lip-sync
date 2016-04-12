#include <iostream>
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
#include "Timeline.h"
#include "Exporter.h"

using std::exception;
using std::string;
using std::vector;
using std::unique_ptr;
using std::make_unique;
using std::map;
using std::chrono::duration;
using std::chrono::duration_cast;
using boost::filesystem::path;

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
		std::throw_with_nested(std::runtime_error(fmt::format("Could not open sound file '{0}'.", filePath.string())));
	}
}

// Tell TCLAP how to handle our types
namespace TCLAP {
	template<>
	struct ArgTraits<LogLevel> {
		typedef ValueLike ValueCategory;
	};
	template<>
	struct ArgTraits<ExportFormat> {
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
	auto exportFormats = vector<ExportFormat>(getEnumValues<ExportFormat>());
	tclap::ValuesConstraint<ExportFormat> exportFormatConstraint(exportFormats);
	tclap::ValueArg<ExportFormat> exportFormat("f", "exportFormat", "The export format.", false, ExportFormat::TSV, &exportFormatConstraint, cmd);
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

		// Export
		unique_ptr<Exporter> exporter;
		switch (exportFormat.getValue()) {
		case ExportFormat::TSV:
			exporter = make_unique<TSVExporter>();
			break;
		case ExportFormat::XML:
			exporter = make_unique<XMLExporter>();
			break;
		case ExportFormat::JSON:
			exporter = make_unique<JSONExporter>();
			break;
		default:
			throw std::runtime_error("Unknown export format.");
		}
		exporter->exportShapes(path(inputFileName.getValue()), shapes, std::cout);

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
