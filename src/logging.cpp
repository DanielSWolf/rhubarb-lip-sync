#include "logging.h"
#include <array>
#include <boost/log/sinks/unlocked_frontend.hpp>
#include <boost/log/expressions.hpp>

using std::string;
using std::lock_guard;
using boost::log::sinks::text_ostream_backend;
using boost::log::record_view;
using boost::log::sinks::unlocked_sink;

namespace expr = boost::log::expressions;

string toString(LogLevel level) {
	constexpr size_t levelCount = static_cast<size_t>(LogLevel::EndSentinel);
	static const std::array<const string, levelCount> strings = {
		"Trace", "Debug", "Info", "Warning", "Error", "Fatal"
	};
	return strings.at(static_cast<size_t>(level));
}

PausableBackendAdapter::PausableBackendAdapter(boost::shared_ptr<text_ostream_backend> backend) :
	backend(backend) {}

void PausableBackendAdapter::consume(const record_view& recordView, const string message) {
	lock_guard<std::mutex> lock(mutex);
	if (isPaused) {
		buffer.push_back(std::make_tuple(recordView, message));
	} else {
		backend->consume(recordView, message);
	}
}

void PausableBackendAdapter::pause() {
	lock_guard<std::mutex> lock(mutex);
	isPaused = true;
}

void PausableBackendAdapter::resume() {
	lock_guard<std::mutex> lock(mutex);
	isPaused = false;
	for (const auto& tuple : buffer) {
		backend->consume(std::get<record_view>(tuple), std::get<string>(tuple));
	}
	buffer.clear();
}

boost::shared_ptr<PausableBackendAdapter> initLogging() {
	// Create logging backend that logs to stderr
	auto streamBackend = boost::make_shared<text_ostream_backend>();
	streamBackend->add_stream(boost::shared_ptr<std::ostream>(&std::cerr, [](std::ostream*) {}));
	streamBackend->auto_flush(true);

	// Create an adapter that allows us to pause, buffer, and resume log output
	auto pausableAdapter = boost::make_shared<PausableBackendAdapter>(streamBackend);

	// Create a sink that feeds into the adapter
	auto sink = boost::make_shared<unlocked_sink<PausableBackendAdapter>>(pausableAdapter);

	// Set output formatting
	sink->set_formatter(expr::stream << "[" << expr::attr<LogLevel>("Severity") << "] " << expr::smessage);

	boost::log::core::get()->add_sink(sink);
	return pausableAdapter;
}

