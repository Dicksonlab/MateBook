#ifndef stringUtilities_hpp
#define stringUtilities_hpp

#include <string>
#include <vector>

std::vector<std::string> split(const std::string& in, char delimiter = '\t');
std::string transpose(const std::string& table);	// transposes a table of tab-separated values

#endif
