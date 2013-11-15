#ifndef pow_hpp
#define pow_hpp

#include <stdexcept>

/*
For classes implementing mathematical primitives, specializations of Zero<T>, One<T>, and so on must be provided if the default implementations don't work as expected.
For instance: a matrix class might have to (partially) specialize One to make One<Mat<T, COLS, ROWS> >::element() return the identity matrix.
Unfortunately, for partial specializations to work, we have to put the functions into class templates, since function templates can't be partially specialized. (Maybe in C++0x?)
*/

// T must be an element of an additive monoid
template<class T>
class Zero {
public:
	static const T& element()
	{
		static T zero;
		return zero;
	}
};

// T must be an element of a multiplicative monoid
template<class T>
class One {
public:
	static const T& element()
	{
		static T one(1);
		return one;
	}
};

// T must be an element of a field
template<class T, bool ZERO_TEST = true>
class Reciprocal {
public:
	static T of(const T& element)
	{
		if (element == Zero<T>::element()) {
			throw std::domain_error("element not invertible");
		}
		return One<T>::element() / element;
	}
};

// T must be an element of a multiplicative group
template<class T>
class Reciprocal<T, false> {
public:
	static T of(const T& element)
	{
		return One<T>::element() / element;
	}
};

template<class B, class E>
class Raise;

// B must be an element of a multiplicative monoid
template<class B>
class Raise<B, unsigned int> {
public:
	Raise(const B& base) :
		base(base)
	{
	}
	
	B toThePowerOf(unsigned int exponent) const
	{
		B ret = One<B>::element();
		B base = this->base;
		while (exponent) {
			if (exponent & 1)
				ret *= base;
			base *= base;
			exponent >>= 1;
		}
		return ret;
	}
	
private:
	const B& base;
};

// B must be an element of a multiplicative group
template<class B>
class Raise<B, int> {
public:
	Raise(const B& base) :
		base(base)
	{
	}
	
	B toThePowerOf(int exponent) const
	{
		bool exponentNegative = exponent < 0;
		if (exponentNegative) {
			exponent = -exponent;
		}
		B ret = One<B>::element();
		B base = this->base;
		while (exponent) {
			if (exponent & 1)
				ret *= base;
			base *= base;
			exponent >>= 1;
		}
		return exponentNegative ? Reciprocal<B>::of(ret) : ret;
	}
	
private:
	const B& base;
};

template<class T>
T powui(const T& base, unsigned int exponent)
{
	return Raise<T, unsigned int>(base).toThePowerOf(exponent);
}

template<class T>
T powi(const T& base, int exponent)
{
	return Raise<T, int>(base).toThePowerOf(exponent);
}

#endif
