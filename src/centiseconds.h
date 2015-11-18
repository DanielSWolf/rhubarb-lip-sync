#ifndef LIPSYNC_CENTISECONDS_H
#define LIPSYNC_CENTISECONDS_H

typedef std::chrono::duration<int, std::centi> centiseconds;

std::ostream& operator <<(std::ostream& stream, const centiseconds cs);

#endif //LIPSYNC_CENTISECONDS_H
