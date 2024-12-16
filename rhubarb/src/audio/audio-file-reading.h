#pragma once

#include <filesystem>
#include <memory>

#include "audio-clip.h"

std::unique_ptr<AudioClip> createAudioFileClip(std::filesystem::path filePath);
