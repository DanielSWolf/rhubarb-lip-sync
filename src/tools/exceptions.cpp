#include "exceptions.h"

using std::string;
using std::exception;

string getMessage(const exception& e) {
	string result(e.what());
	try {
		rethrow_if_nested(e);
	} catch (const exception& innerException) {
		result += "\n" + getMessage(innerException);
	} catch (...) {}

	return result;
}
