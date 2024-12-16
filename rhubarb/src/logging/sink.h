#pragma once

#include "entry.h"

namespace logging {

class Sink {
public:
    virtual ~Sink() = default;
    virtual void receive(const Entry& entry) = 0;
};

} // namespace logging
