#ifndef AttributeCollection_hpp
#define AttributeCollection_hpp

/*
The attributeMap shall be filled in the constructor by derived classes and never be altered after that.
*/

#include "Attribute.hpp"

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include "../../common/source/debug.hpp"

class AttributeCollection {
public:
	AttributeCollection()
	{
	}

	~AttributeCollection()
	{
		for (AttributeMap::iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			delete iter->second;
		}
		attributeMap.clear();
	}

	void reserve(size_t n)
	{
		for (AttributeMap::iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			iter->second->reserve(n);
		}
	}

	std::vector<std::string> getNames() const
	{
		std::vector<std::string> ret;
		ret.reserve(attributeMap.size());
		for (AttributeMap::const_iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			ret.push_back(iter->first);
		}
		return ret;
	}

	template<class T>
	std::vector<std::string> getNames() const
	{
		std::vector<std::string> ret;
		for (AttributeMap::const_iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			if (dynamic_cast<Attribute<T>*>(iter->second)) {
				ret.push_back(iter->first);
			}
		}
		return ret;
	}

	AbstractAttribute& getEmpty(std::string name)
	{
		AbstractAttribute& attribute = get(name);
		if (!attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is not empty");
		}
		return attribute;
	}

	const AbstractAttribute& getEmpty(std::string name) const
	{
		const AbstractAttribute& attribute = get(name);
		if (!attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is not empty");
		}
		return attribute;
	}

	template<class T>
	Attribute<T>& getEmpty(std::string name)
	{
		Attribute<T>& attribute = get<T>(name);
		if (!attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is not empty");
		}
		return attribute;
	}

	template<class T>
	const Attribute<T>& getEmpty(std::string name) const
	{
		const Attribute<T>& attribute = get<T>(name);
		if (!attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is not empty");
		}
		return attribute;
	}

	AbstractAttribute& getFilled(std::string name)
	{
		AbstractAttribute& attribute = get(name);
		if (attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is empty");
		}
		return attribute;
	}

	const AbstractAttribute& getFilled(std::string name) const
	{
		const AbstractAttribute& attribute = get(name);
		if (attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is empty");
		}
		return attribute;
	}

	template<class T>
	Attribute<T>& getFilled(std::string name)
	{
		Attribute<T>& attribute = get<T>(name);
		if (attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is empty");
		}
		return attribute;
	}

	template<class T>
	const Attribute<T>& getFilled(std::string name) const
	{
		const Attribute<T>& attribute = get<T>(name);
		if (attribute.empty()) {
			throw std::logic_error("attribute \"" + name + "\" is empty");
		}
		return attribute;
	}

	AbstractAttribute& get(std::string name)
	{
		AttributeMap::iterator iter = attributeMap.find(name);
		if (iter == attributeMap.end()) {
			throw std::logic_error("attribute \"" + name + "\" not found");
		}
		return *(iter->second);
	}

	const AbstractAttribute& get(std::string name) const
	{
		AttributeMap::const_iterator iter = attributeMap.find(name);
		if (iter == attributeMap.end()) {
			throw std::logic_error("attribute \"" + name + "\" not found");
		}
		return *(iter->second);
	}

	template<class T>
	Attribute<T>& get(std::string name)
	{
		AttributeMap::iterator iter = attributeMap.find(name);
		if (iter == attributeMap.end()) {
			throw std::logic_error("attribute \"" + name + "\" not found");
		}
		return *assert_cast<Attribute<T>*>(iter->second);
	}

	template<class T>
	const Attribute<T>& get(std::string name) const
	{
		AttributeMap::const_iterator iter = attributeMap.find(name);
		if (iter == attributeMap.end()) {
			throw std::logic_error("attribute \"" + name + "\" not found");
		}
		return *assert_cast<Attribute<T>*>(iter->second);
	}

	bool has(std::string name) const
	{
		return attributeMap.find(name) != attributeMap.end();
	}

	template<class T>
	bool has(std::string name) const
	{
		AttributeMap::const_iterator iter = attributeMap.find(name);
		if (iter == attributeMap.end()) {
			return false;
		}
		return dynamic_cast<Attribute<T>*>(iter->second) != NULL;
	}

	void write(std::ostream& out, const std::string& header = "", const char delimiter = '\t') const
	{
		for (AttributeMap::const_iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			if (iter->second->getShortName().empty())  continue;
			out << header << delimiter;
			iter->second->write(out, delimiter);
			out << '\n';
		}
	}

	void writeBinaries(const std::string& outDirPath) const
	{
		for (AttributeMap::const_iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			if (iter->second->getShortName().empty())  continue;
			std::ofstream binaryAttributeFile((outDirPath + "/" + iter->first).c_str(), std::ios::out | std::ios::binary);
			iter->second->writeBinaries(binaryAttributeFile);
		}
	}

	void writeMean(std::ostream& out) const
	{
		for (AttributeMap::const_iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			if (iter->second->getShortName().empty())  continue;
			iter->second->writeMean(out);
			out << '\n';
		}
	}

protected:
	AttributeCollection(const AttributeCollection& copy)
	{
		for (AttributeMap::const_iterator iter = copy.attributeMap.begin(); iter != copy.attributeMap.end(); ++iter) {
			attributeMap[iter->first] = iter->second->clone();
		}
	}

	AttributeCollection& operator=(const AttributeCollection& assign)
	{
		if (this == &assign) {
			return *this;
		}
		for (AttributeMap::iterator iter = attributeMap.begin(); iter != attributeMap.end(); ++iter) {
			delete iter->second;
		}
		attributeMap.clear();
		for (AttributeMap::const_iterator iter = assign.attributeMap.begin(); iter != assign.attributeMap.end(); ++iter) {
			attributeMap[iter->first] = iter->second->clone();
		}
		return *this;
	}

	typedef std::map<std::string, AbstractAttribute*> AttributeMap;
	AttributeMap attributeMap;
};

#endif
