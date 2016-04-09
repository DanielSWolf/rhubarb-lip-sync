#pragma once

#include <boost/log/common.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <string>
#include <vector>
#include <mutex>
#include <tuple>
#include "centiseconds.h"
#include <boost/filesystem.hpp>
#include "tools.h"
#include "enumTools.h"
#include "Timed.h"

enum class LogLevel {
	Trace,
	Debug,
	Info,
	Warning,
	Error,
	Fatal,
	EndSentinel
};

template<>
const std::string& getEnumTypeName<LogLevel>();

template<>
const std::vector<std::tuple<LogLevel, std::string>>& getEnumMembers<LogLevel>();

std::ostream& operator<<(std::ostream& stream, LogLevel value);

std::istream& operator>>(std::istream& stream, LogLevel& value);

using LoggerType = boost::log::sources::severity_logger_mt<LogLevel>;

BOOST_LOG_GLOBAL_LOGGER(globalLogger, LoggerType)

#define LOG(level) \
	BOOST_LOG_STREAM_WITH_PARAMS(globalLogger::get(), (::boost::log::keywords::severity = level))

#define LOG_TRACE LOG(LogLevel::Trace)
#define LOG_DEBUG LOG(LogLevel::Debug)
#define LOG_INFO LOG(LogLevel::Info)
#define LOG_WARNING LOG(LogLevel::Warning)
#define LOG_ERROR LOG(LogLevel::Error)
#define LOG_FATAL LOG(LogLevel::Fatal)

class PausableBackendAdapter :
	public boost::log::sinks::basic_formatted_sink_backend<char, boost::log::sinks::concurrent_feeding>
{
public:
	PausableBackendAdapter(boost::shared_ptr<boost::log::sinks::text_ostream_backend> backend);
	~PausableBackendAdapter();
	void consume(const boost::log::record_view& recordView, const std::string message);
	void pause();
	void resume();
private:
	boost::shared_ptr<boost::log::sinks::text_ostream_backend> backend;
	std::vector<std::tuple<boost::log::record_view, std::string>> buffer;
	std::mutex mutex;
	bool isPaused = false;
};

boost::shared_ptr<PausableBackendAdapter> addPausableStderrSink(LogLevel minLogLevel);

void addFileSink(const boost::filesystem::path& logFilePath, LogLevel minLogLevel);

template<typename TValue>
void logTimedEvent(const std::string& eventName, const Timed<TValue> timedValue) {
	LOG_DEBUG
		<< "##" << eventName << "[" << formatDuration(timedValue.getStart()) << "-" << formatDuration(timedValue.getEnd()) << "]: "
		<< timedValue.getValue();
}

template<typename TValue>
void logTimedEvent(const std::string& eventName, const TimeRange& timeRange, const TValue& value) {
	logTimedEvent(eventName, Timed<TValue>(timeRange, value));
}

template<typename TValue>
void logTimedEvent(const std::string& eventName, centiseconds start, centiseconds end, const TValue& value) {
	logTimedEvent(eventName, Timed<TValue>(start, end, value));
}
