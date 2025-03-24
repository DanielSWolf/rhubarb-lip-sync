#pragma once

#include "logging/Entry.h"
#include "logging/Sink.h"
#include <filesystem>

// Mostly quiet output to stderr.
// Entries are only printed if their log level at least matches the specified minimum level.
class QuietStderrSink : public logging::Sink {
public:
	QuietStderrSink(logging::Level minLevel);
	void receive(const logging::Entry& entry) override;
private:
	logging::Level minLevel;
	bool quietSoFar = true;
	boost::optional<std::filesystem::path> inputFilePath;
	std::shared_ptr<Sink> innerSink;
};
