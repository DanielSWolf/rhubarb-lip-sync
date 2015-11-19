#include <Windows.h>
#include "tools.h"
#include "platform_tools.h"

std::vector<std::wstring> getCommandLineArgs(int argc, char **argv) {
	UNUSED(argv);

	// Get command line as single Unicode string
	LPWSTR commandLine = GetCommandLineW();

	// Split into individual args
	int argumentCount;
	LPWSTR* arguments = CommandLineToArgvW(commandLine, &argumentCount);
	if (!arguments) throw std::runtime_error("Could not determine command line arguments.");
	auto _ = finally([&arguments](){ LocalFree(arguments); });
	assert(argumentCount == argc);

	// Convert to vector
	std::vector<std::wstring> result;
	for (int i = 0; i < argumentCount; i++) {
		result.push_back(arguments[i]);
	}

	return result;
}

boost::filesystem::path getBinDirectory() {
	std::vector<WCHAR> executablePath(MAX_PATH);

	// Try to get the executable path with a buffer of MAX_PATH characters.
	DWORD result = GetModuleFileNameW(0, executablePath.data(), executablePath.size());

	// As long the function returns the buffer size, it is indicating that the buffer
	// was too small. Keep doubling the buffer size until it fits.
	while(result == executablePath.size()) {
		executablePath.resize(executablePath.size() * 2);
		result = GetModuleFileNameW(0, executablePath.data(), executablePath.size());
	}

	// If the function returned 0, something went wrong
	if (result == 0) {
		throw std::runtime_error("Could not determine path of bin directory.");
	}

	return boost::filesystem::path(executablePath.data()).parent_path();
}
