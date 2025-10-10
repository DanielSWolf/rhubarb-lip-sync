#include "tclap/CmdLine.h"
#include <iostream>
#include <string>

using namespace TCLAP;
using namespace std;

int main(int argc, char **argv) {
    CmdLine cmd("test arg conversion operator");
    SwitchArg falseSwitch("f", "false", "test false condition", cmd, false);
    SwitchArg trueSwitch("t", "true", "tests true condition", cmd, true);
    ValueArg<string> strArg("s", "str", "test string arg", false, "defStr",
                            "string", cmd);
    ValueArg<int> intArg("i", "int", "tests int arg", false, 4711, "integer",
                         cmd);

    cmd.parse(argc, argv);

    string s = strArg;
    int i = intArg;

    cout << "for falseSwitch we got : " << falseSwitch << endl
         << "for trueSwitch we got : " << trueSwitch << endl
         << "for strArg we got : " << s << endl
         << "for intArg we got : " << i << endl;
}
