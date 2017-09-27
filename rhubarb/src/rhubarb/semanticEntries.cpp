#include "semanticEntries.h"

using logging::Level;
using std::string;

SemanticEntry::SemanticEntry(Level level, const string& message) :
	Entry(level, message)
{}

StartEntry::StartEntry(const boost::filesystem::path& inputFilePath) :
	SemanticEntry(Level::Info, fmt::format("Application startup. Input file: {}.", inputFilePath)),
	inputFilePath(inputFilePath)
{}

boost::filesystem::path StartEntry::getInputFilePath() const {
	return inputFilePath;
}

ProgressEntry::ProgressEntry(double progress) :
	SemanticEntry(Level::Trace, fmt::format("Progress: {}%", static_cast<int>(progress * 100))),
	progress(progress)
{}

double ProgressEntry::getProgress() const {
	return progress;
}

SuccessEntry::SuccessEntry() :
	SemanticEntry(Level::Info, "Application terminating normally.")
{}

FailureEntry::FailureEntry(const string& reason) :
	SemanticEntry(Level::Fatal, fmt::format("Application terminating with error: {}", reason)),
	reason(reason)
{}

string FailureEntry::getReason() const {
	return reason;
}
