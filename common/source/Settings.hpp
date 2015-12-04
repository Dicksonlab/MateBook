#ifndef Settings_hpp
#define Settings_hpp

/*
A Settings object stores name/value pairs, where the name is a std::string and the value can be anything.
Accessing a value, one has to know the correct type and use the .as<T>() method template.
For convenience, if the value is not of type T, it is assumed to be of type std::string and unstringify<T> is called.
If the value still cannot be cast to T, a std::bad_cast exception is thrown.
*/

#include <stdexcept>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include "serialization.hpp"

class Settings {
	template<class T> class TypedValue;

	class Value {
	public:
		virtual std::string serialize() const = 0;
		virtual void parse(const std::string& s) = 0;

		virtual ~Value()
		{
		}
	};

	template<class T>
	class TypedValue : public Value {
	public:
		TypedValue(boost::function<T ()> getter, boost::function<void (T)> setter) :
			getter(getter),
			setter(setter)
		{
		}

		std::string serialize() const
		{
			return stringify(getter());
		}

		void parse(const std::string& s)
		{
			setter(unstringify<T>(s));
		}

	private:
		boost::function<T ()> getter;
		boost::function<void (T)> setter;
	};

	template<class T>
	class ValueGetter {
	public:
		ValueGetter(const T& value) :
			value(value)
		{
		}

		T operator()() const
		{
			return value;
		}

	private:
		const T& value;
	};

	template<class T>
	class ValueSetter {
	public:
		ValueSetter(T& value) :
			value(value)
		{
		}

		void operator()(T newValue)
		{
			value = newValue;
		}

	private:
		T& value;
	};

public:
	Settings()
	{
	}

	template<class T>
	void add(const std::string name, boost::function<T ()> getter, boost::function<void (T)> setter)
	{
		settings[name] = boost::shared_ptr<Value>(new TypedValue<T>(getter, setter));
	}

	template<class T>
	void add(const std::string name, T& value)
	{
		settings[name] = boost::shared_ptr<Value>(new TypedValue<T>(ValueGetter<T>(value), ValueSetter<T>(value)));
	}

	void importProgramArguments(int argc, char* const argv[])
	{
		std::string settingNamePrefix = "--";
		std::string settingName = "executable";
		std::string settingValue;
		bool expectValue = true;
		for (int  argNum = 0; argNum != argc; ++argNum) {
			std::string arg = argv[argNum];
			if (expectValue) {
				if (arg.find(settingNamePrefix) == 0) {
					// value expected but name found, so we add the name we've found before and then deal with this one
					if (settings.find(settingName) == settings.end()) {
						std::cerr << "warning: program argument \"" << settingName << "\" is not being used" << std::endl;
					} else {
						settings[settingName]->parse(std::string());
					}
					settingName = arg.substr(settingNamePrefix.size());
				} else {
					if (settings.find(settingName) == settings.end()) {
						std::cerr << "warning: program argument \"" << settingName << "\" is not being used" << std::endl;
					} else {
						settings[settingName]->parse(arg);
					}
					expectValue = false;
				}
			} else {
				// we expect the name of a setting
				if (arg.find(settingNamePrefix) == 0) {
					settingName = arg.substr(settingNamePrefix.size());
					expectValue = true;
				} else {
					//throw std::invalid_argument("program argument parsing error: \"" + arg + "\" has to start with \"" + settingNamePrefix + "\" if it is meant to be the name of a setting");
					std::cerr << "warning: program argument \"" << arg << "\" does not start with \"" << settingNamePrefix << "\" and is being ignored" << std::endl;
					continue;
				}
			}
		}
		if (expectValue) {
			// we still expected a value after processing all the arguments so we add the last name we've found
			if (settings.find(settingName) == settings.end()) {
				std::cerr << "warning: program argument \"" << settingName << "\" is not being used" << std::endl;
			} else {
				settings[settingName]->parse(std::string());
			}
		}
	}

	void importFrom(std::istream& in, const char delimiter = '\t')
	{
		// the format is variable\tvalue on each line
		// if no \t is found, we interpret the line as a variable name with the value being the empty string
		// if the line begins with a \t the variable name is the empty string

		Map remaining = settings;

		for (std::string line; std::getline(in, line);) {
			std::string::size_type indexOfDelimiter = line.find(delimiter);
			if (indexOfDelimiter == line.npos) {
				std::string name = line;
				if (settings.find(name) == settings.end()) {
					std::cerr << "warning: setting \"" << name << "\" is not being used" << std::endl;
				} else {
					settings[name]->parse(std::string());
					if (remaining.erase(name) == 0) {
						std::cerr << "warning: duplicate setting \"" << name << "\" encountered, using latest value" << std::endl;
					}
				}
			} else {	// non-empty value string
				std::string name = std::string(line, 0, indexOfDelimiter);
				std::string value = std::string(line, indexOfDelimiter + 1);
				if (settings.find(name) == settings.end()) {
					std::cerr << "warning: setting \"" << name << "\" is not being used" << std::endl;
				} else {
					settings[name]->parse(value);
					if (remaining.erase(name) == 0) {
						std::cerr << "warning: duplicate setting \"" << name << "\" encountered, using latest value" << std::endl;
					}
				}
			}
		}

		for (Map::const_iterator iter = remaining.begin(); iter != remaining.end(); ++iter) {
			std::cerr << "warning: setting \"" << iter->first << "\" has not been set and may be undefined" << std::endl;
		}
	}

	void importFrom(const std::string& fileName, const char delimiter = '\t')
	{
		std::ifstream in(fileName.c_str());
		if (!in) {
			throw std::runtime_error("could not open file for reading: " + fileName);
		}
		importFrom(in, delimiter);
	}

	bool defined(const std::string& name) const
	{
		return settings.find(name) != settings.end();
	}

	void erase(const std::string& name)
	{
		settings.erase(name);
	}

	void clear()
	{
		settings.clear();
	}

	void exportTo(std::ostream& out, const char delimiter = '\t') const
	{
		for (Map::const_iterator iter = settings.begin(); iter != settings.end(); ++iter) {
			out << iter->first << delimiter << iter->second->serialize() << std::endl;
		}
	}

	void exportTo(const std::string& fileName, const char delimiter = '\t') const
	{
		std::ofstream out(fileName.c_str());
		if (!out) {
			throw std::runtime_error("could not open file for writing: " + fileName);
		}
		exportTo(out, delimiter);
	}

private:
	typedef std::map<std::string, boost::shared_ptr<Value> > Map;
	Map settings;
};

// without this specialization the system would break for std::string values that are the empty string
template<>
class Settings::TypedValue<std::string> : public Settings::Value {
public:
	TypedValue(boost::function<std::string ()> getter, boost::function<void (std::string)> setter) :
	getter(getter),
	setter(setter)
	{
	}
	
	std::string serialize() const
	{
		return getter();
	}
	
	void parse(const std::string& s)
	{
		setter(s);
	}
	
private:
	boost::function<std::string ()> getter;
	boost::function<void (std::string)> setter;
};

#endif
