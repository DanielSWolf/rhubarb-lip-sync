#include <iostream>
#include "audio_input/WaveFileReader.h"
#include "phone_extraction.h"

using std::exception;
using std::string;
using std::unique_ptr;

string getMessage(const exception& e) {
	string result(e.what());
	try {
		std::rethrow_if_nested(e);
	} catch(const exception& innerException) {
		result += "\n" + getMessage(innerException);
	} catch(...) {}

	return result;
}

unique_ptr<AudioStream> createAudioStream(string fileName) {
	try {
		return unique_ptr<AudioStream>(new WaveFileReader(fileName));
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Could not open sound file.") );
	}
}

int main(int argc, char *argv[]) {
	try {
		unique_ptr<AudioStream> audioStream = createAudioStream(R"(C:\Users\Daniel\Desktop\audio-test\test 16000Hz 1ch 16bit.wav)");

		std::map<centiseconds, Phone> phones = detectPhones(std::move(audioStream));

		for (auto &pair : phones) {
			std::cout << pair.first << ": " << phoneToString(pair.second) << "\n";
		}

		return 0;
	} catch (const exception& e) {
		std::cout << "An error occurred. " << getMessage(e);
		return 1;
	}
}