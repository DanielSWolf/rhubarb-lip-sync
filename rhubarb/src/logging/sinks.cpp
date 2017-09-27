#include "sinks.h"
#include <iostream>
#include "Entry.h"

using std::string;
using std::lock_guard;
using std::shared_ptr;

namespace logging {

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

}
