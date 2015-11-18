#include <iostream>
#include "audio_input/WaveFileReader.h"
#include "phone_extraction.h"

int main(int argc, char *argv[]) {
	// Create audio stream
	std::unique_ptr<AudioStream> audioStream(
		new WaveFileReader(R"(C:\Users\Daniel\Desktop\audio-test\test 16000Hz 1ch 16bit.wav)"));

	std::map<centiseconds, Phone> phones = detectPhones(std::move(audioStream));

	for (auto& pair : phones) {
		std::cout << pair.first << ": " << phoneToString(pair.second) << "\n";
	}

	return 0;
}