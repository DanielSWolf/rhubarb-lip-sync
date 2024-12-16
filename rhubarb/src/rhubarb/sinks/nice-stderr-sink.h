#pragma once

#include "logging/entry.h"
#include "logging/sink.h"
#include "tools/progress-bar.h"

// Prints nicely formatted progress to stderr.
// Non-semantic entries are only printed if their log level at least matches the specified minimum
// level.
class NiceStderrSink : public logging::Sink {
public:
    NiceStderrSink(logging::Level minLevel);
    void receive(const logging::Entry& entry) override;

private:
    void startProgressIndication();
    void interruptProgressIndication();
    void resumeProgressIndication();

    logging::Level minLevel;
    double progress;
    boost::optional<ProgressBar> progressBar;
    std::shared_ptr<Sink> innerSink;
};
