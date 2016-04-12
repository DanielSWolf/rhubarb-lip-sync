#include "TablePrinter.h"
#include <algorithm>
#include <iomanip>
#include <boost/io/ios_state.hpp>
#include "stringTools.h"

using std::ostream;
using std::initializer_list;
using std::invalid_argument;
using std::vector;
using std::string;

TablePrinter::TablePrinter(ostream *stream, initializer_list<int> columnWidths, int columnSpacing) :
	stream(stream),
	columnWidths(columnWidths.begin(), columnWidths.end()),
	columnSpacing(columnSpacing)
{
	if (stream == nullptr) throw invalid_argument("stream is null.");
	if (columnWidths.size() < 1) throw invalid_argument("No columns defined.");
	if (std::any_of(columnWidths.begin(), columnWidths.end(), [](int width){ return width <= 1; })) {
		throw invalid_argument("All columns must have a width of at least 1.");
	}
	if (columnSpacing < 0) throw invalid_argument("columnSpacing must not be negative.");
}

void TablePrinter::printRow(initializer_list<string> columns) const {
	if (columns.size() != columnWidths.size()) throw invalid_argument("Number of specified strings does not match number of defined columns.");

	// Some cells may span multiple lines.
	// Create matrix of text lines in columns.
	vector<vector<string>> strings(columns.size());
	size_t lineCount = 0;
	{
		int columnIndex = 0;
		for (const string& column : columns) {
			vector<string> lines = wrapString(column, columnWidths[columnIndex], 2);
			if (lines.size() > lineCount) lineCount = lines.size();
			strings[columnIndex] = move(lines);

			columnIndex++;
		}
		// Make sure the matrix is uniform
		for (vector<string>& columnRows : strings) {
			columnRows.resize(lineCount);
		}
	}

	// Save stream flags, restore them at end of scope
	boost::io::ios_flags_saver ifs(*stream);

	// Print lines
	*stream << std::left;
	string spacer(columnSpacing, ' ');
	for (size_t rowIndex = 0; rowIndex < lineCount; rowIndex++) {
		for (size_t columnIndex = 0; columnIndex < columns.size(); columnIndex++) {
			if (columnIndex != 0) {
				*stream << spacer;
			}
			*stream << std::setw(columnWidths[columnIndex]) << strings[columnIndex][rowIndex];
		}
		*stream << std::endl;
	}
}
