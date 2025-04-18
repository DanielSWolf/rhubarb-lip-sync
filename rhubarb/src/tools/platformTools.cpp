#include <filesystem>
#include <format.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "platformTools.h"
#include <whereami.h>
#include <utf8.h>
#include <gsl_util.h>
#include "tools.h"
#include <codecvt>
#include <iostream>

#ifdef _WIN32
	#include <Windows.h>
#endif
#include "fileTools.h"

using std::filesystem::path;
using std::string;
using std::vector;

path _getBinPath() {
	try {
		// Determine path length
		const int pathLength = wai_getExecutablePath(nullptr, 0, nullptr);
		if (pathLength == -1) {
			throw std::runtime_error("Error determining path length.");
		}

		// Get path
		// Note: According to documentation, pathLength does *not* include the trailing zero.
		// Actually, it does.
		// In case there are situations where it doesn't, we allocate one character more.
		std::vector<char> buffer(pathLength + 1);
		if (wai_getExecutablePath(buffer.data(), static_cast<int>(buffer.size()), nullptr) == -1) {
			throw std::runtime_error("Error reading path.");
		}
		buffer[pathLength] = 0;

		// Convert to std::filesystem::path
		const string pathString(buffer.data());
		return path(std::filesystem::canonical(pathString).make_preferred());
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Could not determine path of bin directory."));
	}
}

// Returns the path of the Rhubarb executable binary.
path getBinPath() {
	static const path result = _getBinPath();
	return result;
}

path _getBinDirectory() {
	path binPath = getBinPath();
	path binDirectory = binPath.parent_path();

	// Perform sanity checks on bin directory
	path testPath = binDirectory / "res" / "sphinx" / "cmudict-en-us.dict";
	if (!std::filesystem::exists(testPath)) {
		throw std::runtime_error(fmt::format(
			"Found Rhubarb executable at {}, but could not find resource file {}.",
			binPath.u8string(),
			testPath.u8string()
		));
	}
	try {
		throwIfNotReadable(testPath);
	} catch (...) {
		throw std::runtime_error(fmt::format(
			"Cannot read resource file {}. Please check file permissions.",
			testPath.u8string()
		));
	}

	return binDirectory;
}

// Returns the directory containing the Rhubarb executable binary.
path getBinDirectory() {
	static const path result = _getBinDirectory();
	return result;
}

path getTempFilePath() {
	const path tempDirectory = std::filesystem::temp_directory_path();
	static boost::uuids::random_generator generateUuid;
	const string fileName = to_string(generateUuid());
	return tempDirectory / fileName;
}

std::tm getLocalTime(const time_t& time) {
	tm timeInfo {};
#if (__unix || __linux || __APPLE__)
	localtime_r(&time, &timeInfo);
#else
	localtime_s(&timeInfo, &time);
#endif
	return timeInfo;
}

std::string errorNumberToString(int errorNumber) {
	char message[256];
#if (__unix || __linux || __APPLE__)
	strerror_r(errorNumber, message, sizeof message);
#else
	strerror_s(message, sizeof message, errorNumber);
#endif
	return message;
}

vector<string> argsToUtf8(int argc, char* argv[]) {
#ifdef _WIN32
	// On Windows, there is no way to convert the single-byte argument strings to Unicode.
	// We'll just ignore them.
	UNUSED(argc);
	UNUSED(argv);

	// Get command-line arguments as UTF16 strings
	int argumentCount;
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "Expected wchar_t to be a 16-bit type.");
	char16_t** args =
		reinterpret_cast<char16_t**>(CommandLineToArgvW(GetCommandLineW(), &argumentCount));
	if (!args) {
		throw std::runtime_error("Error splitting the UTF-16 command line arguments.");
	}
	auto freeArgs = gsl::finally([&]() { LocalFree(args); });
	assert(argumentCount == argc);

	// Convert UTF16 strings to UTF8
	vector<string> result;
	for (int i = 0; i < argc; ++i) {
		std::u16string utf16String(args[i]);
		string utf8String;
		utf8::utf16to8(utf16String.begin(), utf16String.end(), back_inserter(utf8String));
		result.push_back(utf8String);
	}
	return result;
#else
	// On Unix systems, command-line args are already in UTF-8 format. Just convert them to strings.
	vector<string> result;
	for (int i = 0; i < argc; ++i) {
		result.push_back(string(argv[i]));
	}
	return result;
#endif
}

class ConsoleBuffer : public std::stringbuf {
public:
	explicit ConsoleBuffer(FILE* file)
		: file(file) {}

	int sync() override {
		fputs(str().c_str(), file);
		str("");
		return 0;
	}

private:
	FILE* file;
};

void useUtf8ForConsole() {
	// Unix systems already expect UTF-8-encoded data
#ifdef _WIN32
	// Set console code page to UTF-8 so the console knows how to interpret string data
	SetConsoleOutputCP(CP_UTF8);

	// Prevent default stream buffer from chopping up UTF-8 byte sequences.
	// See https://stackoverflow.com/questions/45575863/how-to-print-utf-8-strings-to-stdcout-on-windows
	std::cout.rdbuf(new ConsoleBuffer(stdout));
	std::cerr.rdbuf(new ConsoleBuffer(stderr));
#endif
}
