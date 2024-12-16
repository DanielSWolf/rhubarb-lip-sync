#pragma once
#include <filesystem>
#include <fstream>

#include "platform-tools.h"

std::ifstream openFile(std::filesystem::path filePath);

void throwIfNotReadable(std::filesystem::path filePath);
