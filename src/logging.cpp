#include "logging.h"
#include <boost/log/sinks/unlocked_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/keywords/file_name.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
// ReSharper disable once CppUnusedIncludeDirective
#include <boost/log/support/date_time.hpp>
#include <Timed.h>

using std::string;
using std::lock_guard;
using boost::log::sinks::text_ostream_backend;
using boost::log::record_view;
using boost::log::sinks::unlocked_sink;
using std::vector;
using std::tuple;
using std::make_tuple;

namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace attr = boost::log::attributes;

template <>
const string& getEnumTypeName<LogLevel>() {
	static const string name = "LogLevel";
	return name;
}

template <>
const vector<tuple<LogLevel, string>>& getEnumMembers<LogLevel>() {
	static const vector<tuple<LogLevel, string>> values = {
		make_tuple(LogLevel::Trace,		"Trace"),
		make_tuple(LogLevel::Debug,		"Debug"),
		make_tuple(LogLevel::Info,		"Info"),
		make_tuple(LogLevel::Warning,	"Warning"),
		make_tuple(LogLevel::Error,		"Error"),
		make_tuple(LogLevel::Fatal,		"Fatal")
	};
	return values;
}

std::ostream& operator<<(std::ostream& stream, LogLevel value) {
	return stream << enumToString(value);
}

std::istream& operator>>(std::istream& stream, LogLevel& value) {
	string name;
	stream >> name;
	value = parseEnum<LogLevel>(name);
	return stream;
}

PausableBackendAdapter::PausableBackendAdapter(boost::shared_ptr<text_ostream_backend> backend) :
	backend(backend) {}

PausableBackendAdapter::~PausableBackendAdapter() {
	resume();
}

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

BOOST_LOG_GLOBAL_LOGGER_INIT(globalLogger, LoggerType) {
	LoggerType logger;

	logger.add_attribute("TimeStamp", attr::local_clock());

	return logger;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", LogLevel)

boost::shared_ptr<PausableBackendAdapter> addPausableStderrSink(LogLevel minLogLevel) {
	// Create logging backend that logs to stderr
	auto streamBackend = boost::make_shared<text_ostream_backend>();
	streamBackend->add_stream(boost::shared_ptr<std::ostream>(&std::cerr, [](std::ostream*) {}));
	streamBackend->auto_flush(true);

	// Create an adapter that allows us to pause, buffer, and resume log output
	auto pausableAdapter = boost::make_shared<PausableBackendAdapter>(streamBackend);

	// Create a sink that feeds into the adapter
	auto sink = boost::make_shared<unlocked_sink<PausableBackendAdapter>>(pausableAdapter);
	sink->set_formatter(expr::stream << "[" << severity << "] " << expr::smessage);
	sink->set_filter(severity >= minLogLevel);
	boost::log::core::get()->add_sink(sink);

	return pausableAdapter;
}

void addFileSink(const boost::filesystem::path& logFilePath, LogLevel minLogLevel) {
	auto textFileBackend = boost::make_shared<sinks::text_file_backend>(
		keywords::file_name = logFilePath.string());
	auto sink = boost::make_shared<sinks::synchronous_sink<sinks::text_file_backend>>(textFileBackend);
	sink->set_formatter(expr::stream
		<< "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
		<< "] [" << severity << "] " << expr::smessage);
	sink->set_filter(severity >= minLogLevel);
	boost::log::core::get()->add_sink(sink);
}
