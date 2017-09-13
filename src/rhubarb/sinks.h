#pragma once

#include "logging/Entry.h"
#include "logging/Sink.h"
#include "tools/ProgressBar.h"
#include <boost/filesystem/path.hpp>

// Prints nicely formatted progress to stderr.
// Non-semantic entries are only printed if their log level at least matches the specified minimum level.
class NiceStderrSink : public logging::Sink {
public:
	NiceStderrSink(logging::Level minLevel);
	void receive(const logging::Entry& entry) override;
private:
	void startProgressIndication();
	void interruptProgressIndication();
	void resumeProgressIndication();

	logging::Level minLevel;
	boost::optional<ProgressBar> progressBar;
	std::shared_ptr<Sink> innerSink;
};

// Mostly quiet output to stderr.
// Entries are only printed if their log level at least matches the specified minimum level.
class QuietStderrSink : public logging::Sink {
public:
	QuietStderrSink(logging::Level minLevel);
	void receive(const logging::Entry& entry) override;
private:
	logging::Level minLevel;
	bool quietSoFar = true;
	boost::optional<boost::filesystem::path> inputFilePath;
	std::shared_ptr<Sink> innerSink;
};

// Prints machine-readable progress to stderr.
// Non-semantic entries are only printed if their log level at least matches the specified minimum level.
class MachineReadableStderrSink : public logging::Sink {
public:
	MachineReadableStderrSink(logging::Level minLevel);
	void receive(const logging::Entry& entry) override;
private:
	logging::Level minLevel;
	int lastProgressPercent = -1;
};
