#ifndef Attribute_hpp
#define Attribute_hpp

/*
Attributes are properties of a video that change over time. One value is stored per frame.
TODO: Most of the interface should be removed at a later point...we're only providing the vector wrapper during the transition.
*/

#include <stdexcept>
#include <vector>
#include <numeric>
#include <sstream>
#include <iostream>
#include <fstream>
#include "../../common/source/MyBool.hpp"
#include "../../common/source/MyTraits.hpp"
#include "../../common/source/serialization.hpp"
#include "../../common/source/stringUtilities.hpp"
#include "../../common/source/debug.hpp"
#if !defined(MATEBOOK_GUI)
#include "Fly.hpp"
#include "TrackedFrame.hpp"
#endif

template<class T> class Attribute;

template<class T>
class AttributeTraits {
public:
	typedef float Mean;
};

template<class S, size_t N>
class AttributeTraits<Vec<S, N> > {
public:
	typedef Vec<float, N> Mean;
};

class AbstractAttribute {
public:
	virtual ~AbstractAttribute() {}
	virtual AbstractAttribute* clone() const = 0;
	virtual std::string getName() const = 0;
	virtual std::string getType() const = 0;
	virtual std::string getDescription() const = 0;
	virtual bool empty() const = 0;
	virtual size_t size() const = 0;
	virtual void clear() = 0;
	virtual void reserve(size_t n) = 0;
	virtual void resize(size_t n) = 0;
	virtual void write(std::ostream& out, const char delimiter = '\t') const = 0;
	virtual void writeBinaries(std::ostream& out) const = 0;
	virtual void writeMean(std::ostream& out) const = 0;
	virtual void read(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end) = 0;
	virtual void readBinaries(const std::string& filePath) = 0;
	virtual void swap(AbstractAttribute& other) = 0;
	virtual void swap(AbstractAttribute& other, const std::vector<bool>& mask) = 0;
#if !defined(MATEBOOK_GUI)
	virtual void tracked(const Fly& fly) {};
	virtual void tracked(const TrackedFrame& frame) {};
#endif
};

template<class T>
class Attribute : public AbstractAttribute {
public:
	Attribute() : AbstractAttribute()
	{
	}

	Attribute(const std::string& name, const std::string& description, const std::string& unit) : AbstractAttribute(),
		name(name),
		description(description),
		unit(unit)
	{
	}

	AbstractAttribute* clone() const
	{
		Attribute<T>* cloned = new Attribute<T>(name, description, unit);
		cloned->data = data;
		return cloned;
	}

	std::string getName() const
	{
		return name;
	}

	std::string getType() const
	{
		return MyTraits<T>::name();
	}

	std::string getUnit() const
	{
		return unit;
	}

	std::string getDescription() const
	{
		return description;
	}

	const std::vector<T>& getData() const
	{
		return data;
	}

	std::vector<T>& getData()
	{
		return data;
	}

	void push_back(const T& item)
	{
		data.push_back(item);
	}

	Attribute& operator=(const std::vector<T>& assign)
	{
		data = assign;
		return *this;
	}

	typename std::vector<T>::reference front()
	{
		return data.front();
	}

	typename std::vector<T>::const_reference front() const
	{
		return data.front();
	}

	typename std::vector<T>::reference back()
	{
		return data.back();
	}

	typename std::vector<T>::const_reference back() const
	{
		return data.back();
	}

	typename std::vector<T>::reference operator[](size_t index)
	{
		return data[index];
	}

	typename std::vector<T>::const_reference operator[](size_t index) const
	{
		return data[index];
	}

	size_t size() const
	{
		return data.size();
	}

	typename std::vector<T>::iterator begin()
	{
		return data.begin();
	}

	typename std::vector<T>::const_iterator begin() const
	{
		return data.begin();
	}

	typename std::vector<T>::iterator end()
	{
		return data.end();
	}

	typename std::vector<T>::const_iterator end() const
	{
		return data.end();
	}

	void clear()
	{
		data.clear();
	}

	void reserve(size_t n)
	{
		data.reserve(n);
	}

	void resize(size_t n)
	{
		data.resize(n);
	}

	bool empty() const
	{
		return data.empty();
	}

	void write(std::ostream& out, const char delimiter = '\t') const
	{
		out << getName() << delimiter << getType() << delimiter << data.size();
		for (typename std::vector<T>::const_iterator iter = data.begin(); iter != data.end(); ++iter) {
			out << delimiter << *iter;
		}
	}

	void writeBinaries(std::ostream& out) const
	{
		if (!empty()) {
			out.write((char*)&data[0], data.size() * sizeof(T));
		}
	}

	void writeMean(std::ostream& out) const
	{
		typename AttributeTraits<T>::Mean mean = std::accumulate(data.begin(), data.end(), typename AttributeTraits<T>::Mean()) / data.size();
		out << mean;
	}

	void read(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end)
	{
		while (begin != end) {
			data.push_back(unstringify<T>(*begin));
			++begin;
		}
	}

	void readBinaries(const std::string& filePath)
	{
		std::ifstream binaryFile(filePath.c_str(), std::ios::in | std::ios::binary);
		if (binaryFile) {
			binaryFile.seekg(0, std::ios::beg);
			std::streampos fbegin = binaryFile.tellg();
			binaryFile.seekg(0, std::ios::end);
			std::streampos fend = binaryFile.tellg();
			binaryFile.seekg(0, std::ios::beg);
			size_t fileSize = fend - fbegin;	// in bytes

			if (fileSize % sizeof(T) != 0) {
				std::cerr << "error: the size of " << filePath << " is not a multiple of " << stringify(sizeof(T)) << std::endl;
				return;
			}

			if (fileSize != 0) {
				data.resize(fileSize / sizeof(T));
				if (!binaryFile.read((char*)&data[0], fileSize)) {
					std::cerr << "couldn't read the expected number of bytes from " << filePath << std::endl;
				}
			}
		}
	}

	void swap(AbstractAttribute& other)
	{
		Attribute<T>& otherDowncast = *assert_cast<Attribute<T>*>(&other);	//TODO: assert_cast is only defined for pointers
		std::swap(data, otherDowncast.data);
	}

	void swap(AbstractAttribute& other, const std::vector<bool>& mask)
	{
		Attribute<T>& otherDowncast = *assert_cast<Attribute<T>*>(&other);	//TODO: assert_cast is only defined for pointers
		assert(data.size() == otherDowncast.data.size());
		for (size_t i = 0; i != data.size(); ++i) {
			if (mask[i]) {
				// gcc screws this up
				//std::swap(data[i], otherDowncast.data[i]);
				T temp = data[i];
				data[i] = otherDowncast.data[i];
				otherDowncast.data[i] = temp;
			}
		}
	}
    
protected:
	std::string name;
	std::string description;
	std::string unit;
	std::vector<T> data;
};

#if !defined(MATEBOOK_GUI)
template<class T>
class TrackingAttributeFly : public Attribute<T> {
public:
	TrackingAttributeFly(const std::string& name, const std::string& description, const std::string& unit, T (Fly::*getter)() const) : Attribute<T>(name, description, unit),
		getter(getter)
	{
	}

	AbstractAttribute* clone() const
	{
		TrackingAttributeFly<T>* cloned = new TrackingAttributeFly<T>(Attribute<T>::name, Attribute<T>::description, Attribute<T>::unit, getter);
		cloned->data = Attribute<T>::data;
		return cloned;
	}

	void tracked(const Fly& fly)
	{
		Attribute<T>::data.push_back((fly.*getter)());
	};

private:
	T (Fly::*getter)() const;
};

template<class T>
class TrackingAttributeFrame : public Attribute<T> {
public:
	TrackingAttributeFrame(const std::string& name, const std::string& description, const std::string& unit, T (TrackedFrame::*getter)() const) : Attribute<T>(name, description, unit),
		getter(getter)
	{
	}

	AbstractAttribute* clone() const
	{
		TrackingAttributeFrame<T>* cloned = new TrackingAttributeFrame<T>(Attribute<T>::name, Attribute<T>::description, Attribute<T>::unit, getter);
		cloned->data = Attribute<T>::data;
		return cloned;
	}

	void tracked(const TrackedFrame& frame)
	{
		Attribute<T>::data.push_back((frame.*getter)());
	};

private:
	T (TrackedFrame::*getter)() const;
};
#endif

#endif
