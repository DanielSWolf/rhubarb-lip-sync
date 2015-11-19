#include <iostream>
#include "audio_input/WaveFileReader.h"
#include "phone_extraction.h"
#include "platform_tools.h"

using std::exception;
using std::string;
using std::wstring;
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

unique_ptr<AudioStream> createAudioStream(boost::filesystem::path filePath) {
	try {
		return unique_ptr<AudioStream>(new WaveFileReader(filePath));
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Could not open sound file.") );
	}
}

int main(int argc, char *argv[]) {
	try {
		// Get sound file name
		std::vector<wstring> commandLineArgs = getCommandLineArgs(argc, argv);
		if (commandLineArgs.size() != 2) {
			throw std::runtime_error("Invalid command line arguments. Call with sound file name as sole argument.");
		}
		wstring soundFileName = commandLineArgs[1];

		// Create audio streams
		unique_ptr<AudioStream> audioStream = createAudioStream(soundFileName);

		// Detect phones
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