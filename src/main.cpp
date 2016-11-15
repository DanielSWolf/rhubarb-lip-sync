#include <iostream>
#include <format.h>
#include <tclap/CmdLine.h>
#include "appInfo.h"
#include "NiceCmdLineOutput.h"
#include "ProgressBar.h"
#include "logging.h"
#include <gsl_util.h>
#include "Exporter.h"
#include "ContinuousTimeline.h"
#include <boost/filesystem/operations.hpp>
#include "stringTools.h"
#include <boost/range/adaptor/transformed.hpp>
#include <boost/filesystem/fstream.hpp>
#include "parallel.h"
#include "exceptions.h"
#include "textFiles.h"
#include "rhubarbLib.h"

using std::exception;
using std::string;
using std::u32string;
using std::vector;
using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
using std::map;
using std::chrono::duration;
using std::chrono::duration_cast;
using boost::filesystem::path;
using boost::adaptors::transformed;

namespace tclap = TCLAP;

// Tell TCLAP how to handle our types
namespace TCLAP {
	template<>
	struct ArgTraits<logging::Level> {
		typedef ValueLike ValueCategory;
	};
	template<>
	struct ArgTraits<ExportFormat> {
		typedef ValueLike ValueCategory;
	};
}

shared_ptr<logging::PausableSink> addPausableStdErrSink(logging::Level minLevel) {
	auto stdErrSink = make_shared<logging::StdErrSink>(make_shared<logging::SimpleConsoleFormatter>());
	auto pausableSink = make_shared<logging::PausableSink>(stdErrSink);
	auto levelFilter = make_shared<logging::LevelFilter>(pausableSink, minLevel);
	logging::addSink(levelFilter);
	return pausableSink;
}

void addFileSink(path path, logging::Level minLevel) {
	auto file = make_shared<boost::filesystem::ofstream>();
	file->exceptions(std::ifstream::failbit | std::ifstream::badbit);
	file->open(path);
	auto FileSink = make_shared<logging::StreamSink>(file, make_shared<logging::SimpleFileFormatter>());
	auto levelFilter = make_shared<logging::LevelFilter>(FileSink, minLevel);
	logging::addSink(levelFilter);
}

unique_ptr<Exporter> createExporter(ExportFormat exportFormat) {
	switch (exportFormat) {
	case ExportFormat::TSV:
		return make_unique<TSVExporter>();
	case ExportFormat::XML:
		return make_unique<XMLExporter>();
	case ExportFormat::JSON:
		return make_unique<JSONExporter>();
	default:
		throw std::runtime_error("Unknown export format.");
	}
}

int main(int argc, char *argv[]) {
#ifdef _DEBUG
	auto pausableStderrSink = addPausableStdErrSink(logging::Level::Warn);
	pausableStderrSink->pause();
#endif

	// Define command-line parameters
	const char argumentValueSeparator = ' ';
	tclap::CmdLine cmd(appName, argumentValueSeparator, appVersion);
	cmd.setExceptionHandling(false);
	cmd.setOutput(new NiceCmdLineOutput());
	auto logLevels = vector<logging::Level>(logging::LevelConverter::get().getValues());
	tclap::ValuesConstraint<logging::Level> logLevelConstraint(logLevels);
	tclap::ValueArg<logging::Level> logLevel("", "logLevel", "The minimum log level to log", false, logging::Level::Debug, &logLevelConstraint, cmd);
	tclap::ValueArg<string> logFileName("", "logFile", "The log file path.", false, string(), "string", cmd);
	tclap::ValueArg<int> maxThreadCount("", "threads", "The maximum number of worker threads to use.", false, getProcessorCoreCount(), "number", cmd);
	tclap::ValueArg<string> dialogFile("d", "dialogFile", "A file containing the text of the dialog.", false, string(), "string", cmd);
	auto exportFormats = vector<ExportFormat>(ExportFormatConverter::get().getValues());
	tclap::ValuesConstraint<ExportFormat> exportFormatConstraint(exportFormats);
	tclap::ValueArg<ExportFormat> exportFormat("f", "exportFormat", "The export format.", false, ExportFormat::TSV, &exportFormatConstraint, cmd);
	tclap::UnlabeledValueArg<string> inputFileName("inputFile", "The input file. Must be a sound file in WAVE format.", true, "", "string", cmd);

	try {
		auto resumeLogging = gsl::finally([&]() {
			std::cerr << std::endl << std::endl;
#ifdef _DEBUG
			pausableStderrSink->resume();
#endif
			std::cerr << std::endl;
		});

		// Parse command line
		cmd.parse(argc, argv);
		if (maxThreadCount.getValue() < 1) {
			throw std::runtime_error("Thread count must be 1 or higher.");
		}

		// Set up log file
		if (logFileName.isSet()) {
			addFileSink(path(logFileName.getValue()), logLevel.getValue());
		}

		logging::infoFormat("Application startup. Command line: {}", join(
			vector<char*>(argv, argv + argc) | transformed([](char* arg) { return fmt::format("\"{}\"", arg); }), " "));

		std::cerr << "Processing input file.  ";
		ContinuousTimeline<Shape> animation(TimeRange::zero(), Shape::X);
		{
			ProgressBar progressBar;

			// Animate the recording
			animation = animateWaveFile(
				inputFileName.getValue(),
				dialogFile.isSet() ? readUTF8File(path(dialogFile.getValue())) : boost::optional<u32string>(),
				maxThreadCount.getValue(),
				progressBar);
		}
		std::cerr << "Done." << std::endl << std::endl;

		// Export animation
		unique_ptr<Exporter> exporter = createExporter(exportFormat.getValue());
		exporter->exportShapes(path(inputFileName.getValue()), animation, std::cout);

		logging::info("Exiting application normally.");

		return 0;
	} catch (tclap::ArgException& e) {
		// Error parsing command-line args.
		cmd.getOutput()->failure(cmd, e);
		std::cerr << std::endl;
		logging::error("Invalid command line. Exiting application with error code.");
		return 1;
	} catch (tclap::ExitException&) {
		// A built-in TCLAP command (like --help) has finished. Exit application.
		std::cerr << std::endl;
		logging::info("Exiting application after help-like command.");
		return 0;
	} catch (const exception& e) {
		// Generic error
		string message = getMessage(e);
		std::cerr << "An error occurred.\n" << message << std::endl;
		logging::errorFormat("Exiting application with error: {}", message);
		return 1;
	}
}
