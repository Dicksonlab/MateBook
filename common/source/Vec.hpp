#ifndef Vec_hpp
#define Vec_hpp

/*
By design, a few restrictions have been placed on classes implementing mathematical primitives,
to enable their use in arrays and to give guarantees about data layout, which is useful when handing data over to OpenGL:

.) Classes must not contain virtual functions. This prevents the addition of a vpointer to each object.
.) Classes should store their data in arrays where possible to ensure tight packing.
*/

#include <stdexcept>
#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <boost/static_assert.hpp>
#include "algebra.hpp"

template<class S, size_t N>
class Vec : LinearSpaceElement<Vec<S, N>, S> {
public:
	Vec()
	{
		for (size_t i = 0; i != N; ++i) {
			(*this)[i] = Zero<S>::element();
		}
	}

	explicit
	Vec(const S& scalar)
	{
		for (size_t i = 0; i != N; ++i) {
			(*this)[i] = scalar;
		}
	}

	template<class CONVERT_S, size_t RESIZE_N>
	explicit
	Vec(const Vec<CONVERT_S, RESIZE_N>& convert)
	{
		for (size_t i = 0; i != RESIZE_N && i != N; ++i) {
			(*this)[i] = static_cast<S>(convert[i]);
		}
		
		if (N > RESIZE_N) {
			for (size_t i = RESIZE_N; i != N; ++i) {
				(*this)[i] = Zero<S>::element();
			}
		}
	}

	S& operator[](size_t i)
	{
		assert(i < N);
		return data[i];
	}

	const S& operator[](size_t i) const
	{
		assert(i < N);
		return data[i];
	}
	
	template<size_t I>
	S& index()
	{
		BOOST_STATIC_ASSERT(I < N);
		return data[I];
	}

	template<size_t I>
	const S& index() const
	{
		BOOST_STATIC_ASSERT(I < N);
		return data[I];
	}

	S& x()
	{
		return index<0>();
	}

	const S& x() const
	{
		return index<0>();
	}

	S& y()
	{
		return index<1>();
	}

	const S& y() const
	{
		return index<1>();
	}

	S& z()
	{
		return index<2>();
	}

	const S& z() const
	{
		return index<2>();
	}

	S* begin()
	{
		return data;
	}

	const S* begin() const
	{
		return data;
	}

	S* end()
	{
		return data + N;
	}

	const S* end() const
	{
		return data + N;
	}

	bool operator==(const Vec& other) const
	{
		bool ret = true;
		for (size_t i = 0; i != N && ret; ++i) {
			ret = ret && ((*this)[i] == other[i]);
		}
		return ret;
	}

	Vec<S, N>& operator+=(const Vec& other)
	{
		for (size_t i = 0; i != N; ++i) {
			(*this)[i] += other[i];
		}
		return *this;
	}

	Vec<S, N>& operator-=(const Vec& other)
	{
		for (size_t i = 0; i != N; ++i) {
			(*this)[i] -= other[i];
		}
		return *this;
	}

	Vec<S, N>& operator*=(const S& scalar)
	{
		for (size_t i = 0; i != N; ++i) {
			(*this)[i] *= scalar;
		}
		return *this;
	}

	S norm() const
	{
		return sqrt(innerProduct(*this, *this));
	}

	Vec<S, N>& normalize()
	{
		return (*this) /= norm();
	}

protected:

private:
	S data[N];
};

template<class S, size_t N>
Vec<S, N> normalized(const Vec<S, N>& vector)
{
	Vec<S, N> ret = vector;
	return ret.normalize();
}

template<class S, size_t N>
S innerProduct(const Vec<S, N>& left, const Vec<S, N>& right)
{
	S ret = Zero<S>::element();
	for (size_t i = 0; i != N; ++i) {
		ret += left[i] * right[i];
	}
	return ret;
}

template<class S>
Vec<S, 3> crossProduct(const Vec<S, 3>& left, const Vec<S, 3>& right)
{
	Vec<S, 3> ret;
	ret.x() = left.y() * right.z() - left.z() * right.y();
	ret.y() = left.z() * right.x() - left.x() * right.z();
	ret.z() = left.x() * right.y() - left.y() * right.x();
	return ret;
}

template<class S, size_t N>
std::ostream& operator<<(std::ostream& os, const Vec<S, N>& v)
{
/*	std::streamsize widthBefore = os.width();
	std::streamsize precisionBefore = os.precision(5);
	for (size_t i = 0; i != N; ++i) {
		os << std::setw(12) << v[i] << std::setw(widthBefore);
	}
	os << std::endl;
	os.precision(precisionBefore);
	os.width(widthBefore);
	return os;
*/

	for (size_t i = 0; i != N; ++i) {
		os << v[i];
		if (i + 1 != N) {
			os << ' ';
		}
	}
	return os;
}

template<class S>
Vec<S, 2> makeVec(const S& x, const S& y)
{
	Vec<S, 2> ret;
	ret.x() = x;
	ret.y() = y;
	return ret;
}

template<class S>
Vec<S, 3> makeVec(const S& x, const S& y, const S& z)
{
	Vec<S, 3> ret;
	ret.x() = x;
	ret.y() = y;
	ret.z() = z;
	return ret;
}

typedef Vec<float, 2> Vf2;
typedef Vec<float, 3> Vf3;
typedef Vec<float, 4> Vf4;
typedef Vec<double, 2> Vd2;
typedef Vec<double, 3> Vd3;
typedef Vec<double, 4> Vd4;

BOOST_STATIC_ASSERT(sizeof(Vf4) == sizeof(float) * 4);
BOOST_STATIC_ASSERT(sizeof(Vd4) == sizeof(double) * 4);

#endif
