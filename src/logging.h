#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <tuple>
#include "enumTools.h"
#include "tools.h"
#include "Timed.h"

namespace logging {

	enum class Level {
		Trace,
		Debug,
		Info,
		Warn,
		Error,
		Fatal,
		EndSentinel
	};

}

template<>
const std::string& getEnumTypeName<logging::Level>();

template<>
const std::vector<std::tuple<logging::Level, std::string>>& getEnumMembers<logging::Level>();

std::ostream& operator<<(std::ostream& stream, logging::Level value);

std::istream& operator>>(std::istream& stream, logging::Level& value);

namespace logging {

	struct Entry {
		Entry(Level level, const std::string& message);

		time_t timestamp;
		Level level;
		std::string message;
	};

	class Formatter {
	public:
		virtual ~Formatter() = default;
		virtual std::string format(const Entry& entry) = 0;
	};

	class SimpleConsoleFormatter : public Formatter {
	public:
		std::string format(const Entry& entry) override;
	};

	class SimpleFileFormatter : public Formatter {
	public:
		std::string format(const Entry& entry) override;
	private:
		SimpleConsoleFormatter consoleFormatter;
	};

	class Sink {
	public:
		virtual ~Sink() = default;
		virtual void receive(const Entry& entry) = 0;
	};

	class LevelFilter : public Sink {
	public:
		LevelFilter(std::shared_ptr<Sink> innerSink, Level minLevel);
		void receive(const Entry& entry) override;
	private:
		std::shared_ptr<Sink> innerSink;
		Level minLevel;
	};

	class StreamSink : public Sink {
	public:
		StreamSink(std::shared_ptr<std::ostream> stream, std::shared_ptr<Formatter> formatter);
		void receive(const Entry& entry) override;
	private:
		std::shared_ptr<std::ostream> stream;
		std::shared_ptr<Formatter> formatter;
	};

	class StdErrSink : public StreamSink {
	public:
		explicit StdErrSink(std::shared_ptr<Formatter> formatter);
	};

	class PausableSink : public Sink {
	public:
		explicit PausableSink(std::shared_ptr<Sink> innerSink);
		void receive(const Entry& entry) override;
		void pause();
		void resume();
	private:
		std::shared_ptr<Sink> innerSink;
		std::vector<Entry> buffer;
		std::mutex mutex;
		bool isPaused = false;
	};

	void addSink(std::shared_ptr<Sink> sink);

	void log(Level level, const std::string& message);

	template <typename... Args>
	void logFormat(Level level, fmt::CStringRef format, const Args&... args) {
		log(level, fmt::format(format, args...));
	}

#define LOG_WITH_LEVEL(levelName, levelEnum) \
	inline void levelName(const std::string& message) { \
		log(Level::levelEnum, message); \
	} \
	template <typename... Args> \
	void levelName ## Format(fmt::CStringRef format, const Args&... args) { \
		logFormat(Level::levelEnum, format, args...); \
	}

	LOG_WITH_LEVEL(trace, Trace)
	LOG_WITH_LEVEL(debug, Debug)
	LOG_WITH_LEVEL(info, Info)
	LOG_WITH_LEVEL(warn, Warn)
	LOG_WITH_LEVEL(error, Error)
	LOG_WITH_LEVEL(fatal, Fatal)

	template<typename TValue>
	void logTimedEvent(const std::string& eventName, const Timed<TValue> timedValue) {
		debugFormat("##{0} [{1}-{2}]: {3}",
			eventName, formatDuration(timedValue.getStart()), formatDuration(timedValue.getEnd()), timedValue.getValue());
	}

	template<typename TValue>
	void logTimedEvent(const std::string& eventName, const TimeRange& timeRange, const TValue& value) {
		logTimedEvent(eventName, Timed<TValue>(timeRange, value));
	}

	template<typename TValue>
	void logTimedEvent(const std::string& eventName, centiseconds start, centiseconds end, const TValue& value) {
		logTimedEvent(eventName, Timed<TValue>(start, end, value));
	}
}
