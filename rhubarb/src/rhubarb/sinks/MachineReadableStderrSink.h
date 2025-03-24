#pragma once

#include "logging/Entry.h"
#include "logging/Sink.h"

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
