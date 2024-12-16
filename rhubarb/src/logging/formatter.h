#pragma once

#include <string>

#include "entry.h"

namespace logging {

class Formatter {
public:
    virtual ~Formatter() = default;
    virtual std::string format(const Entry& entry) = 0;
};

} // namespace logging
