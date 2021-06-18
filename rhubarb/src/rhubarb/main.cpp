#include <iostream>
#include <format.h>
#include <tclap/CmdLine.h>
#include "core/appInfo.h"
#include "tools/NiceCmdLineOutput.h"
#include "logging/logging.h"
#include "logging/sinks.h"
#include "logging/formatters.h"
#include <gsl_util.h>
#include "exporters/Exporter.h"
#include "time/ContinuousTimeline.h"
#include "tools/stringTools.h"
#include <boost/range/adaptor/transformed.hpp>
#include <fstream>
#include "tools/parallel.h"
#include "tools/exceptions.h"
#include "tools/textFiles.h"
#include "lib/rhubarbLib.h"
#include "ExportFormat.h"
#include "exporters/DatExporter.h"
#include "exporters/TsvExporter.h"
#include "exporters/XmlExporter.h"
#include "exporters/JsonExporter.h"
#include "animation/targetShapeSet.h"
#include <boost/utility/in_place_factory.hpp>
#include "tools/platformTools.h"
#include "sinks.h"
#include "semanticEntries.h"
#include "RecognizerType.h"
#include "recognition/PocketSphinxRecognizer.h"
#include "recognition/PhoneticRecognizer.h"

using std::exception;
using std::string;
using std::string;
using std::vector;
using std::unique_ptr;
using std::make_unique;
using std::shared_ptr;
using std::make_shared;
using std::filesystem::path;
using std::filesystem::u8path;
using boost::adaptors::transformed;
using boost::optional;

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

	template<>
	struct ArgTraits<RecognizerType> {
		typedef ValueLike ValueCategory;
	};
}

shared_ptr<logging::Sink> createFileSink(const path& path, logging::Level minLevel) {
	auto file = make_shared<std::ofstream>();
	file->exceptions(std::ifstream::failbit | std::ifstream::badbit);
	file->open(path);
	auto FileSink =
		make_shared<logging::StreamSink>(file, make_shared<logging::SimpleFileFormatter>());
	return make_shared<logging::LevelFilter>(FileSink, minLevel);
}

unique_ptr<Recognizer> createRecognizer(RecognizerType recognizerType) {
	switch (recognizerType) {
		case RecognizerType::PocketSphinx:
			return make_unique<PocketSphinxRecognizer>();
		case RecognizerType::Phonetic:
			return make_unique<PhoneticRecognizer>();
		default:
			throw std::runtime_error("Unknown recognizer.");
	}
}

unique_ptr<Exporter> createExporter(
	ExportFormat exportFormat,
	const ShapeSet& targetShapeSet,
	double datFrameRate,
	bool datUsePrestonBlair
) {
	switch (exportFormat) {
		case ExportFormat::Dat:
			return make_unique<DatExporter>(targetShapeSet, datFrameRate, datUsePrestonBlair);
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

ShapeSet getTargetShapeSet(const string& extendedShapesString) {
	// All basic shapes are mandatory
	ShapeSet result(ShapeConverter::get().getBasicShapes());

	// Add any extended shapes
	for (char ch : extendedShapesString) {
		Shape shape = ShapeConverter::get().parse(string(1, ch));
		result.insert(shape);
	}
	return result;
}

int main(int platformArgc, char* platformArgv[]) {
	// Set up default logging so early errors are printed to stdout
	const logging::Level defaultMinStderrLevel = logging::Level::Error;
	shared_ptr<logging::Sink> defaultSink = make_shared<NiceStderrSink>(defaultMinStderrLevel);
	logging::addSink(defaultSink);

	// Make sure the console uses UTF-8 on all platforms including Windows
	useUtf8ForConsole();

	// Convert command-line arguments to UTF-8
	const vector<string> args = argsToUtf8(platformArgc, platformArgv);

	// Define command-line parameters
	const char argumentValueSeparator = ' ';
	tclap::CmdLine cmd(appName, argumentValueSeparator, appVersion);
	cmd.setExceptionHandling(false);
	cmd.setOutput(new NiceCmdLineOutput());

	tclap::ValueArg<string> outputFileName(
		"o", "output", "The output file path.",
		false, string(), "string", cmd
	);

	auto logLevels = vector<logging::Level>(logging::LevelConverter::get().getValues());
	tclap::ValuesConstraint<logging::Level> logLevelConstraint(logLevels);
	tclap::ValueArg<logging::Level> logLevel(
		"", "logLevel", "The minimum log level that will be written to the log file",
		false, logging::Level::Debug, &logLevelConstraint, cmd
	);

	tclap::ValueArg<string> logFileName(
		"", "logFile", "The log file path.",
		false, string(), "string", cmd
	);
	tclap::ValueArg<logging::Level> consoleLevel(
		"", "consoleLevel", "The minimum log level that will be printed on the console (stderr)",
		false, defaultMinStderrLevel, &logLevelConstraint, cmd
	);

	tclap::SwitchArg machineReadableMode(
		"", "machineReadable", "Formats all output to stderr in a structured JSON format.",
		cmd, false
	);

	tclap::SwitchArg quietMode(
		"q", "quiet", "Suppresses all output to stderr except for warnings and error messages.",
		cmd, false
	);

	tclap::ValueArg<int> maxThreadCount(
		"", "threads", "The maximum number of worker threads to use.",
		false, getProcessorCoreCount(), "number", cmd
	);

	tclap::ValueArg<string> extendedShapes(
		"", "extendedShapes", "All extended, optional shapes to use.",
		false, "GHX", "string", cmd
	);

	tclap::ValueArg<string> dialogFile(
		"d", "dialogFile", "A file containing the text of the dialog.",
		false, string(), "string", cmd
	);

	tclap::SwitchArg datUsePrestonBlair(
		"", "datUsePrestonBlair", "Only for dat exporter: uses the Preston Blair mouth shape names.",
		cmd, false
	);

	tclap::ValueArg<double> datFrameRate(
		"", "datFrameRate", "Only for dat exporter: the desired frame rate.",
		false, 24.0, "number", cmd
	);

	auto exportFormats = vector<ExportFormat>(ExportFormatConverter::get().getValues());
	tclap::ValuesConstraint<ExportFormat> exportFormatConstraint(exportFormats);
	tclap::ValueArg<ExportFormat> exportFormat(
		"f", "exportFormat", "The export format.",
		false, ExportFormat::Tsv, &exportFormatConstraint, cmd
	);

	auto recognizerTypes = vector<RecognizerType>(RecognizerTypeConverter::get().getValues());
	tclap::ValuesConstraint<RecognizerType> recognizerConstraint(recognizerTypes);
	tclap::ValueArg<RecognizerType> recognizerType(
		"r", "recognizer", "The dialog recognizer.",
		false, RecognizerType::PocketSphinx, &recognizerConstraint, cmd
	);

	tclap::UnlabeledValueArg<string> inputFileName(
		"inputFile", "The input file. Must be a sound file in WAVE format.",
		true, "", "string", cmd
	);

	try {
		// Parse command line
		{
			// TCLAP mutates the function argument! Pass a copy.
			vector<string> argsCopy(args);
			cmd.parse(argsCopy);
		}

		// Set up logging
		// ... to stderr
		if (quietMode.getValue()) {
			logging::addSink(make_shared<QuietStderrSink>(consoleLevel.getValue()));
		} else if (machineReadableMode.getValue()) {
			logging::addSink(make_shared<MachineReadableStderrSink>(consoleLevel.getValue()));
		} else {
			logging::addSink(make_shared<NiceStderrSink>(consoleLevel.getValue()));
		}
		logging::removeSink(defaultSink);
		// ... to log file
		if (logFileName.isSet()) {
			auto fileSink = createFileSink(u8path(logFileName.getValue()), logLevel.getValue());
			logging::addSink(fileSink);
		}

		// Validate and transform command line arguments
		if (maxThreadCount.getValue() < 1) {
			throw std::runtime_error("Thread count must be 1 or higher.");
		}
		path inputFilePath = u8path(inputFileName.getValue());
		ShapeSet targetShapeSet = getTargetShapeSet(extendedShapes.getValue());

		unique_ptr<Exporter> exporter = createExporter(
			exportFormat.getValue(),
			targetShapeSet,
			datFrameRate.getValue(),
			datUsePrestonBlair.getValue()
		);

		logging::log(StartEntry(inputFilePath));
		logging::debugFormat("Command line: {}",
			join(args | transformed([](string arg) { return fmt::format("\"{}\"", arg); }), " "));

		try {
			// On progress change: Create log message
			ProgressForwarder progressSink([](double progress) {
				logging::log(ProgressEntry(progress));
			});

			// Animate the recording
			logging::info("Starting animation.");
			JoiningContinuousTimeline<Shape> animation = animateWaveFile(
				inputFilePath,
				dialogFile.isSet()
					? readUtf8File(u8path(dialogFile.getValue()))
					: boost::optional<string>(),
				*createRecognizer(recognizerType.getValue()),
				targetShapeSet,
				maxThreadCount.getValue(),
				progressSink);
			logging::info("Done animating.");

			// Export animation
			optional<std::ofstream> outputFile;
			if (outputFileName.isSet()) {
				outputFile = boost::in_place(u8path(outputFileName.getValue()));
				outputFile->exceptions(std::ifstream::failbit | std::ifstream::badbit);
			}
			ExporterInput exporterInput = ExporterInput(inputFilePath, animation, targetShapeSet);
			logging::info("Starting export.");
			exporter->exportAnimation(exporterInput, outputFile ? *outputFile : std::cout);
			logging::info("Done exporting.");

			logging::log(SuccessEntry());
		} catch (...) {
			std::throw_with_nested(
				std::runtime_error(fmt::format("Error processing file {}.", inputFilePath.u8string()))
			);
		}

		return 0;
	} catch (tclap::ArgException& e) {
		// Error parsing command-line args.
		cmd.getOutput()->failure(cmd, e);
		logging::log(FailureEntry("Invalid command line."));
		return 1;
	} catch (tclap::ExitException&) {
		// A built-in TCLAP command (like --help) has finished. Exit application.
		logging::info("Exiting application after help-like command.");
		return 0;
	} catch (const exception& e) {
		// Generic error
		string message = getMessage(e);
		logging::log(FailureEntry(message));
		return 1;
	}
}
