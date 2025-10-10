#include <tclap/CmdLine.h>

#include <iostream>

int main() {
  {
    TCLAP::CmdLine cmd("First example");
    TCLAP::SwitchArg first_arg("f", "first", "first switch argument", cmd);
    TCLAP::SwitchArg second_arg("s", "second", "second switch argument", cmd);

    int first_argc = 3;
    const char** first_argv = new const char* [first_argc];
    first_argv[0] = "/nowhere";
    first_argv[1] = "--first";
    first_argv[2] = "--";
    cmd.parse(first_argc,
	      first_argv);
    delete[] first_argv;

    std::cout << "First example: first=" << first_arg.getValue()
	      << ", second=" << second_arg.getValue() << std::endl;
  }

  {
    TCLAP::CmdLine cmd("Second example");
    TCLAP::SwitchArg first_arg("f", "first", "first switch argument", cmd);
    TCLAP::SwitchArg second_arg("s", "second", "second switch argument", cmd);

    int second_argc = 3;
    const char** second_argv = new const char* [second_argc];
    second_argv[0] = "/nowhere";
    second_argv[1] = "--second";
    second_argv[2] = "--";
    cmd.parse(second_argc,
	      second_argv);
    delete[] second_argv;

    std::cout << "Second example: first=" << first_arg.getValue()
	      << ", second=" << second_arg.getValue() << std::endl;
  }
  
  return 0;
}
