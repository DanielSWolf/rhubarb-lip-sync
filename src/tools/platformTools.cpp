#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <format.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "platformTools.h"
#include <whereami.h>
#include <utf8.h>
#include <gsl_util.h>
#include "tools.h"
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp> 
#include <iostream>

#ifdef _WIN32
	#include <Windows.h>
	#include <io.h>
	#include <fcntl.h>
#endif

using boost::filesystem::path;
using std::string;
using std::vector;

path getBinPath() {
	static const path binPath = [] {
		try {
			// Determine path length
			int pathLength = wai_getExecutablePath(nullptr, 0, nullptr);
			if (pathLength == -1) {
				throw std::runtime_error("Error determining path length.");
			}

			// Get path
			// Note: According to documentation, pathLength does *not* include the trailing zero. Actually, it does.
			// In case there are situations where it doesn't, we allocate one character more.
			std::vector<char> buffer(pathLength + 1);
			if (wai_getExecutablePath(buffer.data(), buffer.size(), nullptr) == -1) {
				throw std::runtime_error("Error reading path.");
			}
			buffer[pathLength] = 0;

			// Convert to boost::filesystem::path
			string pathString(buffer.data());
			path result(boost::filesystem::canonical(pathString).make_preferred());
			return result;
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Could not determine path of bin directory."));
		}
	}();
	return binPath;
}

path getBinDirectory() {
	return getBinPath().parent_path();
}

path getTempFilePath() {
	path tempDirectory = boost::filesystem::temp_directory_path();
	static auto generateUuid = boost::uuids::random_generator();
	string fileName = to_string(generateUuid());
	return tempDirectory / fileName;
}

std::tm getLocalTime(const time_t& time) {
	tm timeInfo;
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
	char16_t** args = reinterpret_cast<char16_t**>(CommandLineToArgvW(GetCommandLineW(), &argumentCount));
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

void useUtf8ForBoostFilesystem() {
	std::locale globalLocale = std::locale();
	std::locale utf8Locale(globalLocale, new boost::filesystem::detail::utf8_codecvt_facet);
	path::imbue(utf8Locale);
}
