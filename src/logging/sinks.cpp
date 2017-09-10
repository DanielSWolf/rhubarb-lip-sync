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

}
