#pragma once

#include <initializer_list>
#include <ostream>
#include <vector>

class TablePrinter {
public:
	TablePrinter(std::ostream* stream, std::initializer_list<int> columnWidths, int columnSpacing = 2);
	void printRow(std::initializer_list<std::string> columns) const;
private:
	std::ostream* const stream;
	const std::vector<int> columnWidths;
	const int columnSpacing;
};
