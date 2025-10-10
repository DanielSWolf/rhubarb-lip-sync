// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-
//
// Test that xor args can't be required

#include "tclap/CmdLine.h"

using namespace TCLAP;
using namespace std;

int main(int argc, char **argv) {
    try {
        CmdLine cmd("this is a message", ' ', "0.99");

        ValueArg<string> atest("a", "aaa", "or test a", true, "homer",
                               "string");
        ValueArg<string> btest("b", "bbb", "or test b", false, "homer",
                               "string");
        cmd.xorAdd(atest, btest);

        cmd.parse(argc, argv);
    } catch (SpecificationException &e) {
        std::cout << "Caught SpecificationException: " << e.what() << std::endl;
        return 0;
    }

    return 1;
}
