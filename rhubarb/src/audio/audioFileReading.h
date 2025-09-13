#pragma once

#include <filesystem>
#include <memory>

#include "AudioClip.h"

std::unique_ptr<AudioClip> createAudioFileClip(std::filesystem::path filePath);
