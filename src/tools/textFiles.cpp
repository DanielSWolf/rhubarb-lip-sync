#include "textFiles.h"
#include <boost/filesystem/operations.hpp>
#include <format.h>
#include <boost/filesystem/fstream.hpp>
#include "stringTools.h"

using std::string;
using std::u32string;
using boost::filesystem::path;

u32string readUtf8File(path filePath) {
	if (!exists(filePath)) {
		throw std::invalid_argument(fmt::format("File {} does not exist.", filePath));
	}
	try {
		boost::filesystem::ifstream file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		file.open(filePath);
		string utf8Text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		try {
			return utf8ToUtf32(utf8Text);
		} catch (...) {
			std::throw_with_nested(std::runtime_error(fmt::format("File encoding is not ASCII or UTF-8.", filePath)));
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(fmt::format("Error reading file {0}.", filePath)));
	}
}

