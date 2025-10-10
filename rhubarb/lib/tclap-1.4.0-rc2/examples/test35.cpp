// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  Copyright (c) 2017 Google LLC.
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

#include "tclap/CmdLine.h"

using namespace TCLAP;

int main(int argc, char **argv) {
    CmdLine cmd("");

    ValueArg<int> arg0("a", "a_int", "int arg", false, 4711, "int", cmd);
    ValueArg<int> arg1("b", "b_int", "int arg", false, 4711, "int", cmd);
    arg1.hideFromHelp();

    ValueArg<int> arg2("c", "c_int", "int arg", false, 4711, "int");
    arg2.hideFromHelp();
    ValueArg<int> arg3("d", "d_int", "int arg", false, 4711, "int");
    arg3.hideFromHelp();

    if (argc > 2) {
        arg2.hideFromHelp(false);
    }

    cmd.xorAdd(arg2, arg3);
    cmd.parse(argc, argv);
}
