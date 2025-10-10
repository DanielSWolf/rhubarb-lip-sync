#include <iostream>

#include "tclap/CmdLine.h"

using namespace TCLAP;
using namespace std;

int main() {
    try {
        CmdLine cmd("test constraint bug");
        ValueArg<int> arg("i", "int", "tests int arg", false, 4711, NULL, cmd);
        cout << "Expected exception" << endl;
    } catch (std::logic_error &) { /* expected */
    }

    try {
        CmdLine cmd("test constraint bug");
        ValueArg<int> arg1("i", "int", "tests int arg", false, 4711, NULL,
                           NULL);
        cout << "Expected exception" << endl;
    } catch (std::logic_error &) { /* expected */
    }

    try {
        CmdLine cmd("test constraint bug");
        MultiArg<int> arg1("i", "int", "tests int arg", false, NULL, NULL);
        cout << "Expected exception" << endl;
    } catch (std::logic_error &) { /* expected */
    }

    try {
        CmdLine cmd("test constraint bug");
        MultiArg<int> arg1("i", "int", "tests int arg", false, NULL, cmd);
        cout << "Expected exception" << endl;
    } catch (std::logic_error &) { /* expected */
    }

    cout << "Passed" << endl;
}
