/*
 Test utilities.

 Copyright (c) 2012-2014, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdarg>
#include <cstdio>
#include <string>

#include "posix.h"

enum {BUFFER_SIZE = 256};

#ifdef _MSC_VER
# define FMT_VSNPRINTF vsprintf_s
#else
# define FMT_VSNPRINTF vsnprintf
#endif

template <std::size_t SIZE>
void safe_sprintf(char (&buffer)[SIZE], const char *format, ...) {
  std::va_list args;
  va_start(args, format);
  FMT_VSNPRINTF(buffer, SIZE, format, args);
  va_end(args);
}

// Increment a number in a string.
void increment(char *s);

std::string get_system_error(int error_code);

extern const char *const FILE_CONTENT;

// Opens a buffered file for reading.
fmt::BufferedFile open_buffered_file(FILE **fp = 0);

inline FILE *safe_fopen(const char *filename, const char *mode) {
#if defined(_WIN32) && !defined(__MINGW32__)
  // Fix MSVC warning about "unsafe" fopen.
  FILE *f = 0;
  errno = fopen_s(&f, filename, mode);
  return f;
#else
  return std::fopen(filename, mode);
#endif
}

class TestString {
 private:
  std::string value_;

 public:
  explicit TestString(const char *value = "") : value_(value) {}

  friend std::ostream &operator<<(std::ostream &os, const TestString &s) {
    os << s.value_;
    return os;
  }
};
