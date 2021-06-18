#include "fileTools.h"

#include <cerrno>

using std::filesystem::path;

std::ifstream openFile(path filePath) {
	try {
		std::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(filePath.c_str(), std::ios::binary);

		// Read some dummy data so that we can throw a decent exception in case the file is missing,
		// locked, etc.
		file.seekg(0, std::ios_base::end);
		if (file.tellg()) {
			file.seekg(0);
			file.get();
			file.seekg(0);
		}

		return file;
	} catch (const std::ifstream::failure&) {
		// Error messages on stream exceptions are mostly useless.
		throw std::runtime_error(errorNumberToString(errno));
	}
}

void throwIfNotReadable(path filePath) {
	openFile(filePath);
}
