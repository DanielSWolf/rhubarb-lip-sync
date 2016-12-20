#include "rhubarbLib.h"
#include "Phone.h"
#include "phoneRecognition.h"
#include "textFiles.h"
#include "mouthAnimation.h"
#include "WaveFileReader.h"

using boost::optional;
using std::u32string;
using boost::filesystem::path;
using std::unique_ptr;

JoiningContinuousTimeline<Shape> animateAudioClip(
	const AudioClip& audioClip,
	optional<u32string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink)
{
	BoundedTimeline<Phone> phones = recognizePhones(audioClip, dialog, maxThreadCount, progressSink);
	JoiningContinuousTimeline<Shape> result = animate(phones);
	return result;
}

unique_ptr<AudioClip> createWaveAudioClip(path filePath) {
	try {
		return std::make_unique<WaveFileReader>(filePath);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(fmt::format("Could not open sound file {}.", filePath)));
	}
}

JoiningContinuousTimeline<Shape> animateWaveFile(
	path filePath,
	optional<u32string> dialog,
	int maxThreadCount,
	ProgressSink& progressSink)
{
	return animateAudioClip(*createWaveAudioClip(filePath), dialog, maxThreadCount, progressSink);
}
