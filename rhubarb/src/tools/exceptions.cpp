#include "exceptions.h"

#include <exception>

using std::exception;
using std::string;

string getMessage(const exception& e) {
    string result(e.what());
    try {
        rethrow_if_nested(e);
    } catch (const exception& innerException) {
        result += "\n" + getMessage(innerException);
    } catch (...) {
    }

    return result;
}
