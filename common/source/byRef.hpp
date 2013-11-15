#ifndef byRef_hpp
#define byRef_hpp

/*
A reference-to-value adapter.
To be used to pass parameters by reference to ScopeGuard and similar templates.
*/

template<class T>
class RefHolder {
public:
	RefHolder(T& ref) :
		ref(ref)
	{
	}

	operator T& () const 
	{
		return ref;
	}

private:
    RefHolder& operator=(const RefHolder&);

	T& ref;
};

template<class T>
RefHolder<T> byRef(T& t)
{
	return RefHolder<T>(t);
}

#endif
