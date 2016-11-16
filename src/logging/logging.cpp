#include "logging.h"
#include <tools.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <unordered_map>

using namespace logging;
using std::string;
using std::vector;
using std::shared_ptr;
using std::lock_guard;
using std::unordered_map;

LevelConverter& LevelConverter::get() {
	static LevelConverter converter;
	return converter;
}

string LevelConverter::getTypeName() {
	return "Level";
}

EnumConverter<Level>::member_data LevelConverter::getMemberData() {
	return member_data {
		{ Level::Trace,		"Trace" },
		{ Level::Debug,		"Debug" },
		{ Level::Info,		"Info" },
		{ Level::Warn,		"Warn" },
		{ Level::Error,		"Error" },
		{ Level::Fatal,		"Fatal" }
	};
}

std::ostream& logging::operator<<(std::ostream& stream, Level value) {
	return LevelConverter::get().write(stream, value);
}

std::istream& logging::operator>>(std::istream& stream, Level& value) {
	return LevelConverter::get().read(stream, value);
}

// Returns an int representing the current thread.
// This used to be a simple thread_local variable, but Xcode doesn't support that yet
int getThreadCounter() {
    using thread_id = std::thread::id;

    static std::mutex counterMutex;
    lock_guard<std::mutex> lock(counterMutex);
    
    static unordered_map<thread_id, int> threadCounters;
    static int lastThreadId = 0;
    thread_id threadId = std::this_thread::get_id();
    if (threadCounters.find(threadId) == threadCounters.end()) {
        threadCounters.insert({threadId, ++lastThreadId});
    }
    return threadCounters.find(threadId)->second;
}

Entry::Entry(Level level, const string& message) :
	level(level),
	message(message)
{
	time(&timestamp);
	this->threadCounter = getThreadCounter();
}

string SimpleConsoleFormatter::format(const Entry& entry) {
	return fmt::format("[{0}] {1}", entry.level, entry.message);
}

string SimpleFileFormatter::format(const Entry& entry) {
	return fmt::format("[{0}] {1} {2}", formatTime(entry.timestamp, "%F %H:%M:%S"), entry.threadCounter, consoleFormatter.format(entry));
}

LevelFilter::LevelFilter(shared_ptr<Sink> innerSink, Level minLevel) :
	innerSink(innerSink),
	minLevel(minLevel)
{}

void LevelFilter::receive(const Entry& entry) {
	if (entry.level >= minLevel) {
		innerSink->receive(entry);
	}
}

StreamSink::StreamSink(shared_ptr<std::ostream> stream, shared_ptr<Formatter> formatter) :
	stream(stream),
	formatter(formatter)
{}

void StreamSink::receive(const Entry& entry) {
	string line = formatter->format(entry);
	*stream << line << std::endl;
}

StdErrSink::StdErrSink(shared_ptr<Formatter> formatter) :
	StreamSink(std::shared_ptr<std::ostream>(&std::cerr, [](void*) {}), formatter)
{}

PausableSink::PausableSink(shared_ptr<Sink> innerSink) :
	innerSink(innerSink)
{}

void PausableSink::receive(const Entry& entry) {
	lock_guard<std::mutex> lock(mutex);
	if (isPaused) {
		buffer.push_back(entry);
	} else {
		innerSink->receive(entry);
	}
}

void PausableSink::pause() {
	lock_guard<std::mutex> lock(mutex);
	isPaused = true;

}

void PausableSink::resume() {
	lock_guard<std::mutex> lock(mutex);
	isPaused = false;
	for (const Entry& entry : buffer) {
		innerSink->receive(entry);
	}
	buffer.clear();
}

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
