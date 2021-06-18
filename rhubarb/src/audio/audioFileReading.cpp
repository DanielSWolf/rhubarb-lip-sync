#include "audioFileReading.h"
#include <format.h>
#include "WaveFileReader.h"
#include <boost/algorithm/string.hpp>
#include "OggVorbisFileReader.h"

using std::filesystem::path;
using std::string;
using std::runtime_error;
using fmt::format;

std::unique_ptr<AudioClip> createAudioFileClip(path filePath) {
	try {
		const string extension =
			boost::algorithm::to_lower_copy(filePath.extension().u8string());
		if (extension == ".wav") {
			return std::make_unique<WaveFileReader>(filePath);
		}
		if (extension == ".ogg") {
			return std::make_unique<OggVorbisFileReader>(filePath);
		}
		throw runtime_error(format(
			"Unsupported file extension '{}'. Supported extensions are '.wav' and '.ogg'.",
			extension
		));
	} catch (...) {
		std::throw_with_nested(runtime_error(format("Could not open sound file {}.", filePath.u8string())));
	}
}
