#pragma once

#include <boost/filesystem.hpp>
#include <ctime>
#include <string>

boost::filesystem::path getBinPath();
boost::filesystem::path getBinDirectory();
boost::filesystem::path getTempFilePath();

std::tm getLocalTime(const time_t& time);
std::string errorNumberToString(int errorNumber);

std::vector<std::string> argsToUtf8(int argc, char *argv[]);

void useUtf8ForConsole();
void useUtf8ForBoostFilesystem();