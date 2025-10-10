// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  testing.h
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

#ifndef __TESTING_H__
#define __TESTING_H__

#include <iostream>
#include <sstream>

#define ERROR(t, w) \
    {               \
        t << w;     \
        t.error();  \
    }

class Testing {
public:
    Testing() : errors_(0), msg_() {}

    template <typename T>
    Testing &operator<<(const T &v) {
        msg_ << v;
        return *this;
    }

    void error() {
        errors_++;
        std::cout << msg_.str() << std::endl;
    }

    int errorCount() const { return errors_; }

private:
    int errors_;
    std::ostringstream msg_;
};

#endif
