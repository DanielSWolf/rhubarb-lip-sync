#include "rhubarbLib.h"
#include "core/Phone.h"
#include "recognition/phoneRecognition.h"
#include "tools/textFiles.h"
#include "animation/mouthAnimation.h"
#include "audio/audioFileReading.h"

using boost::optional;
using std::string;
using boost::filesystem::path;
using std::unique_ptr;

JoiningContinuousTimeline<Shape> animateAudioClip(
	const AudioClip& audioClip,
	optional<string> dialog,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink)
{
	BoundedTimeline<Phone> phones = recognizePhones(audioClip, dialog, maxThreadCount, progressSink);
	JoiningContinuousTimeline<Shape> result = animate(phones, targetShapeSet);
	return result;
}

JoiningContinuousTimeline<Shape> animateWaveFile(
	path filePath,
	optional<string> dialog,
	const ShapeSet& targetShapeSet,
	int maxThreadCount,
	ProgressSink& progressSink)
{
	const auto audioClip = createAudioFileClip(filePath);
	return animateAudioClip(*audioClip, dialog, targetShapeSet, maxThreadCount, progressSink);
}
