#include "logging.h"
#include "tools/tools.h"
#include <mutex>
#include "Entry.h"

using namespace logging;
using std::string;
using std::vector;
using std::shared_ptr;
using std::lock_guard;

std::mutex& getLogMutex() {
	static std::mutex mutex;
	return mutex;
}

vector<shared_ptr<Sink>>& getSinks() {
	static vector<shared_ptr<Sink>> sinks;
	return sinks;
}

bool logging::addSink(shared_ptr<Sink> sink) {
	lock_guard<std::mutex> lock(getLogMutex());

	auto& sinks = getSinks();
	if (std::find(sinks.begin(), sinks.end(), sink) == sinks.end()) {
		sinks.push_back(sink);
		return true;
	}
	return false;
}

bool logging::removeSink(std::shared_ptr<Sink> sink) {
	lock_guard<std::mutex> lock(getLogMutex());

	auto& sinks = getSinks();
	const auto it = std::find(sinks.begin(), sinks.end(), sink);
	if (it != sinks.end()) {
		sinks.erase(it);
		return true;
	}
	return false;
}

void logging::log(const Entry& entry) {
	lock_guard<std::mutex> lock(getLogMutex());
	for (auto& sink : getSinks()) {
		sink->receive(entry);
	}
}

void logging::log(Level level, const string& message) {
	const Entry entry = Entry(level, message);
	log(entry);
}
