#pragma once

#include <memory>
#include "AudioClip.h"
#include <boost/filesystem.hpp>

std::unique_ptr<AudioClip> createAudioFileClip(boost::filesystem::path filePath);
