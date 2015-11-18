#include "platform_tools.h"

#include <Windows.h>

boost::filesystem::path getBinDirectory() {
	std::vector<wchar_t> executablePath(MAX_PATH);

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
