#ifndef MyTraits_hpp
#define MyTraits_hpp

#include <string>
#include <stdint.h>

template<class T>
class MyTraits;

//TODO: should this be moved to MyBool.hpp instead?
#include "../../common/source/MyBool.hpp"

template<>
class MyTraits<MyBool> {
public:
	static std::string name()
	{
		return "MyBool";
	}
};

template<>
class MyTraits<float> {
public:
	static std::string name()
	{
		return "float";
	}
};

template<>
class MyTraits<uint32_t> {
public:
	static std::string name()
	{
		return "uint32_t";
	}
};

//TODO: should this be moved to Vec.hpp instead?
#include "../../common/source/Vec.hpp"

//TODO: recursive template to generalize this
template<>
class MyTraits<Vf2> {
public:
	static std::string name()
	{
		return "Vec<float,2>";
	}
};

#endif
