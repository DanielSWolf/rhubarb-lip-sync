#pragma once
#include "platformTools.h"
#include <fstream>

std::ifstream openFile(boost::filesystem::path filePath);

void throwIfNotReadable(boost::filesystem::path filePath);