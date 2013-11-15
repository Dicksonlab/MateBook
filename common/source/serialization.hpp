#ifndef serialization_hpp
#define serialization_hpp

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

template<class T>
std::string stringify(const T& value)
{
	std::ostringstream out;
	if (!(out << value)) {
		throw std::runtime_error("cannot stringify");
	}
	return out.str();
}

template<class T>
T unstringify(const std::string& string)
{
	T value;
	std::istringstream in(string);
	if (!(in >> value)) {
		throw std::runtime_error("cannot unstringify \"" + string + "\"");
	}
	return value;
}

//TODO: should this be moved to Vec.hpp instead?
#include "../../common/source/Vec.hpp"

//TODO: recursive template to generalize this
template<>
inline
Vf2 unstringify<Vf2>(const std::string& string)
{
	float x;
	float y;
	std::istringstream in(string);
	if (!(in >> x >> y)) {
		throw std::runtime_error("cannot unstringify \"" + string + "\"");
	}
	return makeVec(x, y);
}

#endif
