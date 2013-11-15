#ifndef algebra_hpp
#define algebra_hpp

/*
For classes implementing mathematical primitives, specializations of Zero<T>, One<T>, and so on must be provided if the default implementations don't work as expected.
For instance: a matrix class might have to (partially) specialize One to make One<Mat<T, COLS, ROWS> >::element() return the identity matrix.
Unfortunately, for partial specializations to work, we have to put the functions into class templates, since function templates can't be partially specialized.

Further down we have templates (that really are concepts) which shall be used as private bases for classes modelling those concepts.
For instance, when implementing a vector class, one shall privately inherit from LinearSpaceElement, which provides consistent implementations of the operators.
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

// T must define operator==
// T must define operator+=
// T must define a specialization of Zero<T>, unless the default constructor yields the additive identity
// T must define operator-=
// T must define operator*=
// T must define a specialization of One<T>, unless constructing the element via T(1) yields the multiplicative identity
// T must define operator/=
template<class T>
class FieldElement {
	// set
	friend bool operator!=(const T& left, const T& right)
	{
		return !(left == right);
	}

	// additive semigroup
	friend T operator+(const T& left, const T& right)
	{
		T ret = left;
		ret += right;
		return ret;
	}
	
	// additive group
	friend T operator-(const T& left, const T& right)
	{
		T ret = left;
		ret -= right;
		return ret;
	}

	friend T operator-(const T& arg)
	{
		T ret = Zero<T>::element();
		ret -= arg;
		return ret;
	}

	// multiplicative semigroup
	friend T operator*(const T& left, const T& right)
	{
		T ret = left;
		ret *= right;
		return ret;
	}

	// multiplicative group
	friend T operator/(const T& left, const T& right)
	{
		T ret = left;
		ret /= right;
		return ret;
	}
};

// V must define operator==
// V must define operator+=
// V must define a specialization of Zero<V>, unless the default constructor yields the additive identity
// V must define operator-=
// V must define V& operator*=(const S&)
// S must be the type of elements forming a field
// optional (banach space): V may define S V::norm() const
// optional (banach space): V may define V& V::normalize()
// optional (hilbert space): V may define S innerProduct(const V&, const V&)
template<class V, class S>
class LinearSpaceElement {
public:
	// set
	friend bool operator!=(const V& left, const V& right)
	{
		return !(left == right);
	}

	// additive semigroup
	friend V operator+(const V& left, const V& right)
	{
		V ret = left;
		ret += right;
		return ret;
	}
	
	// additive group
	friend V operator-(const V& left, const V& right)
	{
		V ret = left;
		ret -= right;
		return ret;
	}

	friend V operator-(const V& arg)
	{
		V ret = Zero<V>::element();
		ret -= arg;
		return ret;
	}

	// linear space
	friend V& operator/=(V& vector, const S& scalar)
	{
		vector *= Reciprocal<S>::of(scalar);
		return vector;
	}

	friend V operator*(const V& vector, const S& scalar)
	{
		V ret = vector;
		ret *= scalar;
		return ret;
	}

	friend V operator*(const S& scalar, const V& vector)
	{
		return vector * scalar;
	}

	friend V operator/(const V& vector, const S& scalar)
	{
		V ret = vector;
		ret /= scalar;
		return ret;
	}
};

#endif
