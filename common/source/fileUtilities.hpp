#ifndef fileUtilities_hpp
#define fileUtilities_hpp

#include <vector>
#include <string>

std::vector<std::string> ls(std::string directory);

bool isDirectory(std::string path);
bool isFile(std::string path);

bool makeDirectory(std::string path);
bool makePath(std::string path);

bool removeDirectory(std::string path);

#endif
