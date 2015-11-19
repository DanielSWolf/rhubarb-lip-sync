#ifndef LIPSYNC_PLATFORM_TOOLS_H
#define LIPSYNC_PLATFORM_TOOLS_H

#include <boost/filesystem.hpp>

std::vector<std::wstring> getCommandLineArgs(int argc, char *argv[]);

boost::filesystem::path getBinDirectory();

#endif //LIPSYNC_PLATFORM_TOOLS_H
