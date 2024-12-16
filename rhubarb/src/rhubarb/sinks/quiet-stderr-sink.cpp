#include "quiet-stderr-sink.h"

#include <iostream>

#include "core/app-info.h"
#include "logging/formatters.h"
#include "logging/sinks.h"
#include "semantic-entries.h"

using logging::Level;
using logging::SimpleConsoleFormatter;
using logging::StdErrSink;
using std::make_shared;
using std::string;

QuietStderrSink::QuietStderrSink(Level minLevel) :
    minLevel(minLevel),
    innerSink(make_shared<StdErrSink>(make_shared<SimpleConsoleFormatter>())) {}

void QuietStderrSink::receive(const logging::Entry& entry) {
    // Set inputFilePath as soon as we get it
    if (const auto* startEntry = dynamic_cast<const StartEntry*>(&entry)) {
        inputFilePath = startEntry->getInputFilePath();
    }

    if (entry.level >= minLevel) {
        if (quietSoFar) {
            // This is the first message we print. Give a bit of context.
            const string intro = inputFilePath
                ? fmt::format(
                      "{} {} processing file {}:", appName, appVersion, inputFilePath->u8string()
                  )
                : fmt::format("{} {}:", appName, appVersion);
            std::cerr << intro << std::endl;
            quietSoFar = false;
        }
        innerSink->receive(entry);
    }
}
