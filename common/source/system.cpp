#include "system.hpp"

#include <cstdlib>
#include <stdexcept>

std::string getUserName()
{
	const char* userName = NULL;

	#if defined(_WIN32)
		userName = std::getenv("USERNAME");
	#else
		userName = std::getenv("USER");
	#endif

	if (!userName) {
		throw std::runtime_error("the user name is not defined in the environment");
	}

	return std::string(userName);
}
