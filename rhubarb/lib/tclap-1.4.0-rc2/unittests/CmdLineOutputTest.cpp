// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  CmdLineOutputTest.cpp
 *
 *  Copyright (c) 2021, Google LLC
 *  All rights reserved.
 *
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#include "tclap/CmdLineOutput.h"
#include "testing.h"

using namespace TCLAP;

struct BasenameCase {
    BasenameCase(const char *name, const char *basename)
        : name(name), basename(basename) {}
    const char *name;
    const char *basename;
};

struct RemoveSuffixCase {
    RemoveSuffixCase(const char *str, const char *suffix, const char *want)
        : str(str), suffix(suffix), want(want) {}
    const char *str, *suffix, *want;
};

void TestRemoveSuffix(Testing &t) {
    RemoveSuffixCase tests[] = {
        RemoveSuffixCase("", "foo", ""),
        RemoveSuffixCase("foo", "foo", ""),
        RemoveSuffixCase("foobar", "foo", "foobar"),
        RemoveSuffixCase("foobar", "bar", "foo"),
        RemoveSuffixCase("afoo", "foo", "a"),
        RemoveSuffixCase("fooa", "foo", "fooa"),
    };

    for (unsigned i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        const RemoveSuffixCase &test = tests[i];
        std::string got = removeSuffix(test.str, test.suffix);
        if (got != test.want) {
            ERROR(t, "removeSuffix(" << test.str << ", " << test.suffix
                                     << "), got \"" << got << "\", want \""
                                     << test.want << '"');
        }
    }
}

void TestBasename(Testing &t) {
    BasenameCase tests[] = {
        BasenameCase("", ""),
        BasenameCase("./a/b/c", "c"),
        BasenameCase("a/b/c", "c"),
        BasenameCase("/a/b/c", "c"),
        BasenameCase("/foo/bar/bazilicum.out", "bazilicum.out"),
        BasenameCase("/foo/bar/baz.exe", "baz"),
        BasenameCase("/foo/bar/baz", "baz"),
#ifndef _WIN32
        BasenameCase("foo\\bar\\baz", "foo\\bar\\baz"),
#endif
    };

    for (unsigned i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        BasenameCase &test = tests[i];
        std::string name = test.name;
#ifdef _WIN32
        std::replace(name.begin(), name.end(), '/', '\\');
#endif
        std::string s = basename(name);
        if (s != test.basename) {
            ERROR(t, "basename: got \"" << s << "\", want \"" << test.basename
                                        << "\"");
        }
    }
}

int main() {
    Testing t;
    TestBasename(t);
    TestRemoveSuffix(t);
    return t.errorCount();
}
