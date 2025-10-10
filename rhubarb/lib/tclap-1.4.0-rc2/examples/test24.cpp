// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

// Test various Arg properties such as invalid flag/names

#include "tclap/CmdLine.h"

using namespace TCLAP;
using namespace std;

int main() {
    CmdLine cmd("Command description message", ' ', "0.9");
    try {  // Argument with two character 'flag'
        ValueArg<string> nameArg("nx", "name", "Name to print", true, "homer",
                                 "string");
        return EXIT_FAILURE;
    } catch (SpecificationException &e) {
        cout << e.what() << std::endl;  // Expected
    }

    try {  // space as flag
        ValueArg<string> nameArg(" ", "name", "Name to print", true, "homer",
                                 "string");
        return EXIT_FAILURE;
    } catch (SpecificationException &e) {
        cout << e.what() << std::endl;  // Expected
    }

    try {  // - as flag
        ValueArg<string> nameArg("-", "name", "Name to print", true, "homer",
                                 "string");
        return EXIT_FAILURE;
    } catch (SpecificationException &e) {
        cout << e.what() << std::endl;  // Expected
    }

    try {  // -- as flag
        ValueArg<string> nameArg("--", "name", "Name to print", true, "homer",
                                 "string");
        return EXIT_FAILURE;
    } catch (SpecificationException &e) {
        cout << e.what() << std::endl;  // Expected
    }

    try {  // space as name
        ValueArg<string> nameArg("n", " ", "Name to print", true, "homer",
                                 "string");
        return EXIT_FAILURE;
    } catch (SpecificationException &e) {
        cout << e.what() << std::endl;  // Expected
    }

    try {  // - as flag
        ValueArg<string> nameArg("n", "-", "Name to print", true, "homer",
                                 "string");
        return EXIT_FAILURE;
    } catch (SpecificationException &e) {
        cout << e.what() << std::endl;  // Expected
    }

    try {  // -- as flag
        ValueArg<string> nameArg("n", "--", "Name to print", true, "homer",
                                 "string");
        return EXIT_FAILURE;
    } catch (SpecificationException &e) {
        cout << e.what() << std::endl;  // Expected
    }
}
