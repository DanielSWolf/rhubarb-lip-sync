#pragma once

#include "centiseconds.h"
#include "logging/logging.h"
#include "time-range.h"
#include "timed.h"

template <typename TValue>
void logTimedEvent(const std::string& eventName, const Timed<TValue> timedValue) {
    logging::debugFormat(
        "##{0}[{1}-{2}]: {3}",
        eventName,
        formatDuration(timedValue.getStart()),
        formatDuration(timedValue.getEnd()),
        timedValue.getValue()
    );
}

template <typename TValue>
void logTimedEvent(const std::string& eventName, const TimeRange& timeRange, const TValue& value) {
    logTimedEvent(eventName, Timed<TValue>(timeRange, value));
}

template <typename TValue>
void logTimedEvent(
    const std::string& eventName, centiseconds start, centiseconds end, const TValue& value
) {
    logTimedEvent(eventName, Timed<TValue>(start, end, value));
}
