#include "QuietStderrSink.h"
#include "logging/sinks.h"
#include "logging/formatters.h"
#include "semanticEntries.h"
#include "core/appInfo.h"
#include <iostream>

using std::string;
using std::make_shared;
using logging::Level;
using logging::StdErrSink;
using logging::SimpleConsoleFormatter;

QuietStderrSink::QuietStderrSink(Level minLevel) :
	minLevel(minLevel),
	innerSink(make_shared<StdErrSink>(make_shared<SimpleConsoleFormatter>()))
{
}

void QuietStderrSink::receive(const logging::Entry& entry) {
	// Set inputFilePath as soon as we get it
	if (const auto* startEntry = dynamic_cast<const StartEntry*>(&entry)) {
		inputFilePath = startEntry->getInputFilePath();
	}

	if (entry.level >= minLevel) {
		if (quietSoFar) {
			// This is the first message we print. Give a bit of context.
			const string intro = inputFilePath
				? fmt::format("{} {} processing file {}:", appName, appVersion, inputFilePath->u8string())
				: fmt::format("{} {}:", appName, appVersion);
			std::cerr << intro << std::endl;
			quietSoFar = false;
		}
		innerSink->receive(entry);
	}
}
