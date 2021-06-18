#pragma once
#include "platformTools.h"
#include <fstream>
#include <filesystem>

std::ifstream openFile(std::filesystem::path filePath);

void throwIfNotReadable(std::filesystem::path filePath);