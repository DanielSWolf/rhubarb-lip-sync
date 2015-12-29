#ifndef LIPSYNC_MOUTH_ANIMATION_H
#define LIPSYNC_MOUTH_ANIMATION_H

#include <map>
#include "Phone.h"
#include "centiseconds.h"
#include "Shape.h"

std::map<centiseconds, Shape> animate(const std::map<centiseconds, Phone>& phones);

#endif //LIPSYNC_MOUTH_ANIMATION_H
