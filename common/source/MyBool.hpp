#ifndef MyBool_hpp
#define MyBool_hpp

/*
This class provides a wrapper around bool to allow for platform-independent serialization and to work around the std::vector<bool> specialization.
*/

#include <stdint.h>
#include <iostream>

class MyBool {
	friend std::ostream& operator<<(std::ostream& os, MyBool myBool);
	friend std::istream& operator>>(std::istream& is, MyBool& myBool);

public:
	inline
	MyBool()
	{
	}

	inline
	MyBool(bool convert) :
		value(convert)
	{
	}

	inline
	operator bool() const
	{
		return value;
	}

private:
	uint8_t value;
};

inline
std::ostream& operator<<(std::ostream& os, MyBool myBool)
{
	os << (bool)myBool.value;
	return os;
}

inline
std::istream& operator>>(std::istream& is, MyBool& myBool)
{
	bool temp;
	is >> temp;
	myBool = temp;
	return is;
}

#endif
