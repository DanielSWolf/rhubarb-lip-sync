#pragma once

#include <filesystem>
#include <ctime>
#include <string>
#include <vector>

std::filesystem::path getBinPath();
std::filesystem::path getBinDirectory();
std::filesystem::path getTempFilePath();

std::tm getLocalTime(const time_t& time);
std::string errorNumberToString(int errorNumber);

std::vector<std::string> argsToUtf8(int argc, char* argv[]);

void useUtf8ForConsole();
