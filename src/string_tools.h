#ifndef RHUBARB_LIP_SYNC_STRING_TOOLS_H
#define RHUBARB_LIP_SYNC_STRING_TOOLS_H

#include <string>
#include <vector>

std::vector<std::string> splitIntoLines(const std::string& s);

std::vector<std::string> wrapSingleLineString(const std::string& s, int lineLength);

std::vector<std::string> wrapString(const std::string& s, int lineLength);

#endif //RHUBARB_LIP_SYNC_STRING_TOOLS_H
