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

enum class LogLevel {
	Trace,
	Debug,
	Info,
	Warning,
	Error,
	Fatal,
	EndSentinel
};

std::string toString(LogLevel level);

template<typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<< (std::basic_ostream<CharT, TraitsT>& stream, LogLevel level) {
	return stream << toString(level);
}

using LoggerType = boost::log::sources::severity_logger_mt<LogLevel>;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(globalLogger, LoggerType)

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

boost::shared_ptr<PausableBackendAdapter> initLogging();

void logTimedEvent(const std::string& eventName, centiseconds start, centiseconds end, const std::string& value);
