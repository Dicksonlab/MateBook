/**
 *  @brief	an iterator that skips a given number of values
 */

#ifndef StrideIterator_hpp
#define StrideIterator_hpp

#include <iterator>
#include <cassert>
#include <boost/operators.hpp> // consistently defines other operators (!=, <=, >, >=, ...) using the ones we supply (==, <, ...)

template<class RandomAccessIterator>
class StrideIterator : public boost::random_access_iteratable<
	StrideIterator<RandomAccessIterator>,
	typename std::iterator_traits<RandomAccessIterator>::pointer,
	typename std::iterator_traits<RandomAccessIterator>::difference_type,
	typename std::iterator_traits<RandomAccessIterator>::reference> {
public:
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
	typedef typename std::iterator_traits<RandomAccessIterator>::reference reference;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_type;
	typedef typename std::iterator_traits<RandomAccessIterator>::pointer pointer;
	typedef std::random_access_iterator_tag iterator_category;
	typedef StrideIterator self;

	StrideIterator() :
		adaptedIter(),
		offset(),
		step()
	{
	}

	StrideIterator(const self& copy) :
		adaptedIter(copy.adaptedIter),
		offset(copy.offset),
		step(copy.step)
	{
	}

	StrideIterator(RandomAccessIterator iterToAdapt, difference_type step) :
		adaptedIter(iterToAdapt),
		offset(),
		step(step)
	{
	}

	self& operator++()
	{
		offset += step;
		return *this;
	}

	self& operator--()
	{
		offset -= step;
		return *this;
	}

	self& operator+=(difference_type forward)
	{
		offset += forward * step;
		return *this;
	}

	self& operator-=(difference_type back)
	{
		offset -= back * step;
		return *this;
	}

	reference operator*()
	{
		return *(adaptedIter + offset);
	}

	friend bool operator==(const self& left, const self& right)
	{
		assert(left.adaptedIter == right.adaptedIter);
		assert(left.step == right.step);
		return left.offset == right.offset;
	}

	friend bool operator<(const self& left, const self& right)
	{
		assert(left.adaptedIter == right.adaptedIter);
		assert(left.step == right.step);
		return left.offset < right.offset;
	}

	friend difference_type operator-(const self& left, const self& right)
	{
		assert(left.adaptedIter == right.adaptedIter);
		assert(left.step == right.step);
		assert((left.offset - right.offset) % left.step == 0);
		return (left.offset - right.offset) / left.step;
	}

private:
	RandomAccessIterator adaptedIter;
	difference_type offset;
	difference_type step;
};


/**
 *	@brief	convenient StrideIterator creation through automatic type deduction
 */
template<class RandomAccessIterator>
inline
StrideIterator<RandomAccessIterator> makeStrideIterator(RandomAccessIterator iterToAdapt, typename std::iterator_traits<RandomAccessIterator>::difference_type step)
{
	return StrideIterator<RandomAccessIterator>(iterToAdapt, step);
}

#endif
