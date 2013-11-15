#include "stringUtilities.hpp"
#include <sstream>
#include <vector>
#include <stdexcept>

std::vector<std::string> split(const std::string& in, char delimiter)
{
	std::vector<std::string> ret;
	std::istringstream inStream(in);
	for (std::string item; std::getline(inStream, item, delimiter); ret.push_back(item));
	return ret;
}

std::string transpose(const std::string& table)
{
	if (table.empty()) {
		return table;
	}

	size_t tabsPerLine = 0;
	bool tabsPerLineKnown = false;
	std::vector<size_t> valueBegins(1, 0);	// the first value begins at index 0
	size_t tabsInThisLine = 0;
	size_t lineCount = 0;
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i] == '\t') {
			valueBegins.push_back(i + 1);
			++tabsInThisLine;
		} else if (table[i] == '\n') {
			valueBegins.push_back(i + 1);
			++lineCount;
			if (!tabsPerLineKnown) {
				tabsPerLine = tabsInThisLine;
				tabsPerLineKnown = true;
			} else {
				// sanity check requiring the same number of tabs in each line
				if (tabsInThisLine != tabsPerLine) {
					throw std::runtime_error("transpose: not the same number of tabs per line");
				}
			}
			tabsInThisLine = 0;
		}
	}

	// in case the file didn't end in '\n' we're adding an extra valueBegin
	//TODO: the sanity check above doesn't cover the last line if it doesn't end in \n
	if ((1 + tabsPerLine) * lineCount == valueBegins.size()) {
		valueBegins.push_back(table.size());
		++lineCount;
	}

	std::ostringstream out;
	for (size_t row = 0; row != (1 + tabsPerLine); ++row) {
		for (size_t col = 0; col != lineCount; ++col) {
			size_t begin = valueBegins[col * (1 + tabsPerLine) + row];
			size_t end = valueBegins[col * (1 + tabsPerLine) + row + 1] - 1;	//TODO: do we have to adjust by -1 if end is the end of line because of \r on windows?
			out << table.substr(begin, end - begin);
			if (col + 1 != lineCount) {
				out << '\t';
			}
		}
		out << '\n';
	}

	return out.str();
}
