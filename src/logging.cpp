#include "logging.h"
#include <tools.h>
#include <iostream>

using namespace logging;
using std::string;
using std::vector;
using std::tuple;
using std::make_tuple;
using std::shared_ptr;
using std::lock_guard;

template <>
const string& getEnumTypeName<Level>() {
	static const string name = "LogLevel";
	return name;
}

template <>
const vector<tuple<Level, string>>& getEnumMembers<Level>() {
	static const vector<tuple<Level, string>> values = {
		make_tuple(Level::Trace,	"Trace"),
		make_tuple(Level::Debug,	"Debug"),
		make_tuple(Level::Info,		"Info"),
		make_tuple(Level::Warn,		"Warn"),
		make_tuple(Level::Error,	"Error"),
		make_tuple(Level::Fatal,	"Fatal")
	};
	return values;
}

std::ostream& operator<<(std::ostream& stream, Level value) {
	return stream << enumToString(value);
}

std::istream& operator>>(std::istream& stream, Level& value) {
	string name;
	stream >> name;
	value = parseEnum<Level>(name);
	return stream;
}

Entry::Entry(Level level, const string& message) :
	level(level),
	message(message)
{
	time(&timestamp);
}

string SimpleConsoleFormatter::format(const Entry& entry) {
	return fmt::format("[{0}] {1}", entry.level, entry.message);
}

string SimpleFileFormatter::format(const Entry& entry) {
	return fmt::format("[{0}] {1}", formatTime(entry.timestamp, "%F %H:%M:%S"), consoleFormatter.format(entry));
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
	StreamSink(std::shared_ptr<std::ostream>(&std::cerr, [](void*){}), formatter)
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
	for (auto& sink : getSinks()) {
		sink->receive(Entry(level, message));
	}
}
