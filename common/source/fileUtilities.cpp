#include "fileUtilities.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/progress.hpp>

namespace fs = boost::filesystem;

std::vector<std::string> ls(std::string directory)
{
	std::vector<std::string> entries;
	for (fs::directory_iterator iter(directory); iter != fs::directory_iterator(); ++iter) {
		entries.push_back(iter->path().filename().string());
	}
	return entries;
}

bool isDirectory(std::string path)
{
	return fs::is_directory(fs::path(path));
}

bool isFile(std::string path)
{
	return fs::is_regular_file(fs::path(path));
}

bool makeDirectory(std::string path)
{
	return fs::create_directory(fs::path(path));
}

// create all directories in path that do not exist
bool makePath(std::string path)
{
	return fs::create_directories(fs::path(path));
}

bool removeDirectory(std::string path)
{
	return fs::remove_all(fs::path(path)) > 0;
}
