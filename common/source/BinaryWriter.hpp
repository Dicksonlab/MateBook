#ifndef BinaryWriter_hpp
#define BinaryWriter_hpp

/*
A helper class for writing binary file formats.
*/

#include <iostream>

class BinaryWriter {
public:
	inline
	BinaryWriter(std::ostream& os) :
		os(os)
	{
	}	

	template<class T>
	BinaryWriter& writeFrom(const T& source)
	{
		os.write(reinterpret_cast<const char*>(&source), sizeof(source));
		return *this;
	}

	template<class T>
	BinaryWriter& skip(T& source)
	{
		skipBytes(sizeof(T));
		return *this;
	}

	inline
	BinaryWriter& skipBytes(size_t byteCount)
	{
		while (byteCount--) {
			os.put(0);
		}
		return *this;
	}
	
	inline
	#pragma warning(push)
	#pragma warning(disable: 4800)	// warning C4800: 'void *' : forcing value to bool 'true' or 'false' (performance warning)
	operator bool() const
	{
		return os;
	}
	#pragma warning(pop)

private:
	std::ostream& os;
};

#endif
