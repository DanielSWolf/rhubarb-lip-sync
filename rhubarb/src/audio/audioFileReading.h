#pragma once

#include <memory>
#include "AudioClip.h"
#include <filesystem>

std::unique_ptr<AudioClip> createAudioFileClip(std::filesystem::path filePath);
