#include "audio-file-reading.h"

#include <format.h>

#include <boost/algorithm/string.hpp>

#include "ogg-vorbis-file-reader.h"
#include "wave-file-reader.h"

using std::runtime_error;
using std::string;
using std::filesystem::path;

std::unique_ptr<AudioClip> createAudioFileClip(path filePath) {
    try {
        const string extension = boost::algorithm::to_lower_copy(filePath.extension().u8string());
        if (extension == ".wav") {
            return std::make_unique<WaveFileReader>(filePath);
        }
        if (extension == ".ogg") {
            return std::make_unique<OggVorbisFileReader>(filePath);
        }
        throw runtime_error(fmt::format(
            "Unsupported file extension '{}'. Supported extensions are '.wav' and '.ogg'.",
            extension
        ));
    } catch (...) {
        std::throw_with_nested(
            runtime_error(fmt::format("Could not open sound file {}.", filePath.u8string()))
        );
    }
}
