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
#include "ExportFormat.h"
#include "TsvExporter.h"
#include "XmlExporter.h"
#include "JsonExporter.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>

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
	case ExportFormat::Tsv:
		return make_unique<TsvExporter>();
	case ExportFormat::Xml:
		return make_unique<XmlExporter>();
	case ExportFormat::Json:
		return make_unique<JsonExporter>();
	default:
		throw std::runtime_error("Unknown export format.");
	}
}

int main(int argc, char *argv[]) {
	auto pausableStderrSink = addPausableStdErrSink(logging::Level::Warn);
	pausableStderrSink->pause();

	// Define command-line parameters
	const char argumentValueSeparator = ' ';
	tclap::CmdLine cmd(appName, argumentValueSeparator, appVersion);
	cmd.setExceptionHandling(false);
	cmd.setOutput(new NiceCmdLineOutput());
	auto logLevels = vector<logging::Level>(logging::LevelConverter::get().getValues());
	tclap::ValuesConstraint<logging::Level> logLevelConstraint(logLevels);
	tclap::ValueArg<logging::Level> logLevel("", "logLevel", "The minimum log level to log", false, logging::Level::Debug, &logLevelConstraint, cmd);
	tclap::ValueArg<string> logFileName("", "logFile", "The log file path.", false, string(), "string", cmd);
	tclap::SwitchArg quietMode("q", "quiet", "Suppresses all output to stderr except for error messages.", cmd, false);
	tclap::ValueArg<int> maxThreadCount("", "threads", "The maximum number of worker threads to use.", false, getProcessorCoreCount(), "number", cmd);
	tclap::ValueArg<string> dialogFile("d", "dialogFile", "A file containing the text of the dialog.", false, string(), "string", cmd);
	auto exportFormats = vector<ExportFormat>(ExportFormatConverter::get().getValues());
	tclap::ValuesConstraint<ExportFormat> exportFormatConstraint(exportFormats);
	tclap::ValueArg<ExportFormat> exportFormat("f", "exportFormat", "The export format.", false, ExportFormat::Tsv, &exportFormatConstraint, cmd);
	tclap::UnlabeledValueArg<string> inputFileName("inputFile", "The input file. Must be a sound file in WAVE format.", true, "", "string", cmd);

	std::ostream* infoStream = &std::cerr;
	boost::iostreams::stream<boost::iostreams::null_sink> nullStream((boost::iostreams::null_sink()));

	try {
		auto resumeLogging = gsl::finally([&]() {
			*infoStream << std::endl << std::endl;
			pausableStderrSink->resume();
		});

		// Parse command line
		cmd.parse(argc, argv);
		if (quietMode.getValue()) {
			infoStream = &nullStream;
		}
		if (maxThreadCount.getValue() < 1) {
			throw std::runtime_error("Thread count must be 1 or higher.");
		}
		path inputFilePath(inputFileName.getValue());

		// Set up log file
		if (logFileName.isSet()) {
			addFileSink(path(logFileName.getValue()), logLevel.getValue());
		}

		logging::infoFormat("Application startup. Command line: {}", join(
			vector<char*>(argv, argv + argc) | transformed([](char* arg) { return fmt::format("\"{}\"", arg); }), " "));

		try {
			*infoStream << fmt::format("Generating lip-sync data for {}.", inputFilePath) << std::endl;
			*infoStream << "Processing.  ";
			JoiningContinuousTimeline<Shape> animation(TimeRange::zero(), Shape::X);
			{
				ProgressBar progressBar(*infoStream);

				// Animate the recording
				animation = animateWaveFile(
					inputFilePath,
					dialogFile.isSet() ? readUtf8File(path(dialogFile.getValue())) : boost::optional<u32string>(),
					maxThreadCount.getValue(),
					progressBar);
			}
			*infoStream << "Done." << std::endl << std::endl;

			// Export animation
			unique_ptr<Exporter> exporter = createExporter(exportFormat.getValue());
			exporter->exportShapes(inputFilePath, animation, std::cout);

			logging::info("Exiting application normally.");
		} catch (...) {
			std::throw_with_nested(std::runtime_error(fmt::format("Error processing file {}.", inputFilePath)));
		}

		return 0;
	} catch (tclap::ArgException& e) {
		// Error parsing command-line args.
		cmd.getOutput()->failure(cmd, e);
		logging::error("Invalid command line. Exiting application.");
		return 1;
	} catch (tclap::ExitException&) {
		// A built-in TCLAP command (like --help) has finished. Exit application.
		logging::info("Exiting application after help-like command.");
		return 0;
	} catch (const exception& e) {
		// Generic error
		string message = getMessage(e);
		logging::fatalFormat("Exiting application with error:\n{}", message);
		return 1;
	}
}
