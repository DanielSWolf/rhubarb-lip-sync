#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/predef.h>
#include <format.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "platformTools.h"

using boost::filesystem::path;
using std::string;

constexpr int InitialBufferSize = 256;

#if (BOOST_OS_CYGWIN || BOOST_OS_WINDOWS)

	#include <Windows.h>

	path getBinPathImpl() {
		std::vector<WCHAR> buffer(InitialBufferSize);

		// Try to get the executable path with a buffer of MAX_PATH characters.
		DWORD result = GetModuleFileNameW(0, buffer.data(), buffer.size());

		// As long the function returns the buffer size, it is indicating that the buffer
		// was too small. Keep doubling the buffer size until it fits.
		while (result == buffer.size()) {
			buffer.resize(buffer.size() * 2);
			result = GetModuleFileNameW(0, buffer.data(), buffer.size());
		}

		// If the function returned 0, something went wrong
		if (result == 0) {
			DWORD error = GetLastError();
			throw std::runtime_error(fmt::format("`GetModuleFileNameW` failed. Error code: {0}", error));
		}

		return path(buffer.data());
	}

#elif (BOOST_OS_MACOS)

	#include <mach-o/dyld.h>

	path getBinPathImpl() {
		std::vector<char> buffer(InitialBufferSize);
		uint32_t size = buffer.size();
		int result = _NSGetExecutablePath(buffer.data(), &size);
		if (result == -1) {
			// Insufficient buffer. Resize.
			buffer.resize(size);
			result = _NSGetExecutablePath(buffer.data(), &size);
		}
		if (result != 0) {
			throw std::runtime_error(fmt::format("`_NSGetExecutablePath` failed. Error code: {0}", result));
		}
		return path(buffer.data());
	}

#elif (BOOST_OS_SOLARIS)

	#include <stdlib.h>

	path getBinPathImpl() {
		const char* executableName = getexecname();
		if (!executableName) {
			throw std::runtime_error("`getexecname` failed."));
		}
		return path(executableName);
	}

#elif (BOOST_OS_BSD)

	#include <sys/sysctl.h>
	#include <errno.h>

	path getBinPathImpl() {
		// Determine required buffer size
		std::array<int, 4> mib = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
		size_t size = 0;
		sysctl(mib.data(), mib.size(), nullptr, &size, nullptr, 0);

		// Get path
		std::vector<char> buffer(size);
		int result = sysctl(mib.data(), mib.size(), buffer.data(), &size, nullptr, 0);
		if (result == -1) {
			throw std::runtime_error(fmt::format("`sysctl` failed. Error code: {0}", errno));
		}

		return path(std::string(buffer, size));
	}

#elif (BOOST_OS_LINUX)

	#include <unistd.h>
	#include <sys/stat.h>

	path getBinPathImpl() {
		// Determine required buffer size
		stat info;
		const char* selfPath = "/proc/self/exe";
		int result = lstat(selfPath, &stat);
		if (result == -1) {
			throw std::runtime_error(fmt::format("`lstat` failed. Error code: {0}", errno));
		}

		// Get path
		std::vector<char> buffer(info.st_size);
		result = readlink(selfPath, buffer.data(), buffer.size());
		if (result == -1) {
			throw std::runtime_error(fmt::format("`readlink` failed. Error code: {0}", errno));
		}

		return path(std::string(buffer, buffer.size()));
	}

#else

	#error "Unsupported platform."

#endif

path getBinPath() {
	try {
		static path binPath(boost::filesystem::canonical(getBinPathImpl()).make_preferred());
		return binPath;
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Could not determine path of bin directory.") );
	}
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
