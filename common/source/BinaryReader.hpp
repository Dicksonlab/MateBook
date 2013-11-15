#ifndef BinaryReader_hpp
#define BinaryReader_hpp

/*
A helper class for reading binary file formats.

The user can either provide the std::istream which must not be deleted while in use by BinaryReader, or a file name.
In the latter case the ifstream is owned by the BinaryReader object.
*/

#include <iostream>
#include <string>
#include <vector>

class BinaryReader {
public:
	inline
	BinaryReader(std::istream& is) :
		ownedIs(NULL),
		is(is)
	{
	}

	inline
	BinaryReader(const std::string& fileName) :
		ownedIs(new std::ifstream(fileName.c_str(), std::ios::in | std::ios::binary)),
		is(*ownedIs)
	{
		if (!is) {
			delete ownedIs;
			throw std::runtime_error(std::string("Failed to open binary file \"") + fileName + "\" for reading.");
		}
	}

	template<class T>
	BinaryReader& readInto(T& target)
	{
		is.read(reinterpret_cast<char*>(&target), sizeof(target));
		return *this;
	}
/*
	template<class T>
	BinaryReader& readInto(T*& target)
	{
		is.read(reinterpret_cast<char*>(&target), sizeof(target));
		return *this;
	}

	template<class T, size_t N>
	BinaryReader& readInto(T (&target)[N])
	{
		is.read(reinterpret_cast<char*>(&target), sizeof(target));
		return *this;
	}
*/
	template<class T>
	BinaryReader& readInto(std::vector<T>& target)
	{
		if (target.empty()) {
			return *this;
		}

		is.read(reinterpret_cast<char*>(&target[0]), sizeof(T) * target.size());
		return *this;
	}

	template<class T>
	BinaryReader& skip(T& target)
	{
		is.ignore(sizeof(T));
		return *this;
	}

	inline
	BinaryReader& skipBytes(size_t byteCount)
	{
		is.ignore(byteCount);
		return *this;
	}
	
	#pragma warning(push)
	#pragma warning(disable: 4800)	// warning C4800: 'void *' : forcing value to bool 'true' or 'false' (performance warning)
	inline
	operator bool() const
	{
		return is;
	}
	#pragma warning(pop)

	inline
	~BinaryReader()
	{
		delete ownedIs;
	}

private:
	BinaryReader(const BinaryReader&);
	BinaryReader& operator=(const BinaryReader&);

	std::istream* ownedIs;
	std::istream& is;
};

#endif
