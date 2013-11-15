#ifndef Singleton_hpp
#define Singleton_hpp

/*
This Singleton template supports three different ways of accessing objects:

1. Accessing an unnamed instance through instance().
2. Accessing different instances using names known at compile-time via instance<instanceName>().
3. Accessing different instances using names known at run-time via instance("instanceName").

Using the second option, one should define an enum in the header file where the class that is to be instantiated is defined (or globally for built-in types) like so:

enum SomeClass::singleton {zero, one, two};

Using this definition, one can access an individual object like this:

Singleton<SomeClass>::instance<SomeClass::two>();
*/

#include <string>
#include <map>

template<class T>
class Singleton {
public:
	static T& instance()
	{
		static T singleton;
		return singleton;
	}

	template<unsigned int N>
	static T& instance()
	{
		static T singleton;
		return singleton;
	}

	static T& instance(std::string name)
	{
		static std::map<std::string, T> runtimeSingletons;
		return runtimeSingletons[name];
	}

protected:

private:
};

#endif
