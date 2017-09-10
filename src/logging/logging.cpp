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

void logging::addSink(shared_ptr<Sink> sink) {
	lock_guard<std::mutex> lock(getLogMutex());
	getSinks().push_back(sink);
}

void logging::log(Level level, const string& message) {
	lock_guard<std::mutex> lock(getLogMutex());
	const Entry entry = Entry(level, message);
	for (auto& sink : getSinks()) {
		sink->receive(entry);
	}
}
