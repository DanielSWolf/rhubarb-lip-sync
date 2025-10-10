#!/usr/bin/python

# Copyright (c) 2018 Google LLC
# All rights reserved.
#
# See the file COPYING in the top directory of this distribution for
# more information.
#
# THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import sys
import re

def process_file(path):
    args = None
    out = None
    test = None

    lines = []

    with open(path) as f:
        for line in f.readlines():
            if line.startswith('#'):
                lines.append(line)
                continue

            m = re.match(r'../examples/(test[0-9]+) (.*) > tmp.out.*', line)
            if m:
                (test, args) = m.groups()
                lines.append("./simple-test.sh `basename $0 .sh` %s %s\n"
                             % (test, args))
                continue

            m = re.match(r'../examples/(test[0-9]+) > tmp.out.*', line)
            if m:
                test = m.group(1)
                args = ""
                lines.append("./simple-test.sh `basename $0 .sh` %s\n"
                             % test)

                continue

            m = re.match(r'.*(test[0-9]+).out.*', line)
            if m:
                out = m.group(1)

    if not all([v != None for v in [out, test, args]]):
        print "Failed to parse", path
        print out, test, args
        return

    with open(path, 'w') as f:
        for line in lines:
            f.write(line)

for path in sys.argv[1:]:
    process_file(path)
