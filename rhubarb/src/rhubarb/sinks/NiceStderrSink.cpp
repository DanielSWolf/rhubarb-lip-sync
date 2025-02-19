#include "NiceStderrSink.h"
#include "logging/sinks.h"
#include "logging/formatters.h"
#include "semanticEntries.h"
#include <boost/utility/in_place_factory.hpp>

using std::make_shared;
using logging::Level;
using logging::StdErrSink;
using logging::SimpleConsoleFormatter;

NiceStderrSink::NiceStderrSink(Level minLevel) :
	minLevel(minLevel),
	progress(0.0),
	innerSink(make_shared<StdErrSink>(make_shared<SimpleConsoleFormatter>()))
{
}

void NiceStderrSink::receive(const logging::Entry& entry) {
	// For selected semantic entries, print a user-friendly message instead of
	// the technical log message.
	if (const auto* startEntry = dynamic_cast<const StartEntry*>(&entry)) {
		std::cerr
			<< fmt::format("Generating lip sync data for {}.", startEntry->getInputFilePath().u8string())
			<< std::endl;
		startProgressIndication();
	}
	else if (const auto* progressEntry = dynamic_cast<const ProgressEntry*>(&entry)) {
		assert(progressBar);
		progress = progressEntry->getProgress();
		progressBar->reportProgress(progress);
	}
	else if (dynamic_cast<const SuccessEntry*>(&entry)) {
		interruptProgressIndication();
		std::cerr << "Done." << std::endl;
	}
	else {
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
}

void NiceStderrSink::interruptProgressIndication() {
	progressBar.reset();
	std::cerr << std::endl;
}

void NiceStderrSink::resumeProgressIndication() {
	std::cerr << "Progress (cont'd): ";
	progressBar = boost::in_place(progress);
}
