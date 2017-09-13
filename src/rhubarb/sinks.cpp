#include "sinks.h"
#include "logging/sinks.h"
#include "logging/formatters.h"
#include "semanticEntries.h"
#include "tools/stringTools.h"
#include "core/appInfo.h"
#include <boost/utility/in_place_factory.hpp>

using std::string;
using std::make_shared;
using logging::Level;
using logging::LevelFilter;
using logging::StdErrSink;
using logging::SimpleConsoleFormatter;
using boost::optional;

NiceStderrSink::NiceStderrSink(Level minLevel) :
	minLevel(minLevel),
	innerSink(make_shared<StdErrSink>(make_shared<SimpleConsoleFormatter>()))
{}

void NiceStderrSink::receive(const logging::Entry& entry) {
	// For selected semantic entries, print a user-friendly message instead of the technical log message.
	if (const StartEntry* startEntry = dynamic_cast<const StartEntry*>(&entry)) {
		std::cerr << fmt::format("Generating lip sync data for {}.", startEntry->getInputFilePath()) << std::endl;
		startProgressIndication();
	} else if (const ProgressEntry* progressEntry = dynamic_cast<const ProgressEntry*>(&entry)) {
		assert(progressBar);
		progressBar->reportProgress(progressEntry->getProgress());
	} else if (dynamic_cast<const SuccessEntry*>(&entry)) {
		interruptProgressIndication();
		std::cerr << "Done." << std::endl;
	} else {
		// Treat the entry as a normal log message
		if (entry.level >= minLevel) {
			const bool inProgress = progressBar.is_initialized();
			if (inProgress) interruptProgressIndication();
			innerSink->receive(entry);
			if (inProgress) resumeProgressIndication();
		}
	}
}

void NiceStderrSink::startProgressIndication() {
	std::cerr << "Progress: ";
	progressBar = boost::in_place();
	progressBar->setClearOnDestruction(false);
}

void NiceStderrSink::interruptProgressIndication() {
	progressBar.reset();
	std::cerr << std::endl;
}

void NiceStderrSink::resumeProgressIndication() {
	std::cerr << "Progress (cont'd): ";
	progressBar = boost::in_place();
	progressBar->setClearOnDestruction(false);
}

QuietStderrSink::QuietStderrSink(Level minLevel) :
	minLevel(minLevel),
	innerSink(make_shared<StdErrSink>(make_shared<SimpleConsoleFormatter>()))
{}

void QuietStderrSink::receive(const logging::Entry& entry) {
	// Set inputFilePath as soon as we get it
	if (const StartEntry* startEntry = dynamic_cast<const StartEntry*>(&entry)) {
		inputFilePath = startEntry->getInputFilePath();
	}

	if (entry.level >= minLevel) {
		if (quietSoFar) {
			// This is the first message we print. Give a bit of context.
			const string intro = inputFilePath
				? fmt::format("{} {} processing file {}:", appName, appVersion, *inputFilePath)
				: fmt::format("{} {}:", appName, appVersion);
			std::cerr << intro << std::endl;
			quietSoFar = false;
		}
		innerSink->receive(entry);
	}
}

MachineReadableStderrSink::MachineReadableStderrSink(Level minLevel) :
	minLevel(minLevel)
{}

string formatLogProperty(const logging::Entry& entry) {
	return fmt::format(R"("log": {{ "level": "{}", "message": "{}" }})", entry.level, escapeJsonString(entry.message));
}

void MachineReadableStderrSink::receive(const logging::Entry& entry) {
	optional<string> line;
	if (dynamic_cast<const SemanticEntry*>(&entry)) {
		if (const StartEntry* startEntry = dynamic_cast<const StartEntry*>(&entry)) {
			const string file = escapeJsonString(startEntry->getInputFilePath().string());
			line = fmt::format(R"({{ "type": "start", "file": "{}", {} }})", file, formatLogProperty(entry));
		} else if (const ProgressEntry* progressEntry = dynamic_cast<const ProgressEntry*>(&entry)) {
			const int progressPercent = static_cast<int>(progressEntry->getProgress() * 100);
			if (progressPercent > lastProgressPercent) {
				line = fmt::format(R"({{ "type": "progress", "value": {:.2f}, {} }})", progressEntry->getProgress(), formatLogProperty(entry));
				lastProgressPercent = progressPercent;
			}
		} else if (dynamic_cast<const SuccessEntry*>(&entry)) {
			line = fmt::format(R"({{ "type": "success", {} }})", formatLogProperty(entry));
		} else if (const FailureEntry* failureEntry = dynamic_cast<const FailureEntry*>(&entry)) {
			const string reason = escapeJsonString(failureEntry->getReason());
			line = fmt::format(R"({{ "type": "failure", "reason": "{}", {} }})", reason, formatLogProperty(entry));
		} else {
			throw std::runtime_error("Unsupported type of semantic entry.");
		}
	} else {
		if (entry.level >= minLevel) {
			line = fmt::format(R"({{ "type": "log", {} }})", formatLogProperty(entry));
		}
	}

	if (line) {
		std::cerr << *line << std::endl;
	}
}
