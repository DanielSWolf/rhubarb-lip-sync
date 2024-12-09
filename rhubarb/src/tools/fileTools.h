#pragma once
#include <filesystem>
#include <fstream>

#include "platformTools.h"

std::ifstream openFile(std::filesystem::path filePath);

void throwIfNotReadable(std::filesystem::path filePath);
