#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/predef.h>
#include <format.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "platformTools.h"
#include <whereami.h>

using boost::filesystem::path;
using std::string;

path getBinPath() {
	try {
		// Determine path length
		int pathLength = wai_getExecutablePath(nullptr, 0, nullptr);
		if (pathLength == -1) {
			throw std::runtime_error("Error determining path length.");
		}
		
		// Get path
		std::vector<char> buffer(pathLength);
		if (wai_getExecutablePath(buffer.data(), buffer.size(), nullptr) == -1) {
			throw std::runtime_error("Error reading path.");
		}
		
		// Convert to boost::filesystem::path
		string pathString(buffer.data(), buffer.size());
		static path binPath(boost::filesystem::canonical(pathString).make_preferred());
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

std::tm getLocalTime(const time_t& time) {
	tm timeInfo;
#if (BOOST_OS_UNIX || BOOST_OS_MACOS)
	localtime_r(&time, &timeInfo);
#else
	localtime_s(&timeInfo, &time);
#endif
	return timeInfo;
}

std::string errorNumberToString(int errorNumber) {
	char message[256];
#if (BOOST_OS_UNIX || BOOST_OS_MACOS)
	strerror_r(errorNumber, message, sizeof message);
#else
	strerror_s(message, sizeof message, errorNumber);
#endif
	return message;
}
