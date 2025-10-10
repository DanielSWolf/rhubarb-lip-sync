// Example based on question in https://sourceforge.net/p/tclap/support-requests/2/
//
// Shows how to use a pair as a "custom type". Note that the argument
// flag must be specified, like -p "1 2.3" (with quotes).

#include <iostream>
#include <utility>

// We need to tell TCLAP how to parse our pair, we assume it will be
// given as two arguments separated by whitespace.
std::istream &operator>>(std::istream &is, std::pair<int, double> &p) {
  return is >> p.first >> p.second;
}

// Make it easy to print values of our type.
std::ostream &operator<<(std::ostream &os, const std::pair<int, double> &p) {
  return os << p.first << ' ' << p.second;
}

#include "tclap/CmdLine.h"

using namespace TCLAP;

// Our pair can now be used as any other type.
int main(int argc, char **argv) {
  CmdLine cmd("test pair argument");
  ValueArg<std::pair<int, double> > parg("p", "pair", "int,double pair",
                                         true,
                                         std::make_pair(0, 0.0),
                                         "int,double",
                                         cmd);
  cmd.parse(argc, argv);
  std::cout << parg.getValue() << std::endl;
}
