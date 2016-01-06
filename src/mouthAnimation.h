#pragma once

#include <map>
#include "Phone.h"
#include "centiseconds.h"
#include "Shape.h"

std::map<centiseconds, Shape> animate(const std::map<centiseconds, Phone>& phones);
