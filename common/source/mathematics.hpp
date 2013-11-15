#ifndef mathematics_hpp
#define mathematics_hpp

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <set>

namespace constant {
	const double E = 2.71828182845904523536;
	const double PI = 3.14159265358979323846;
}

template<class T>
T log2(const T& number)
{
	return std::log(number) / std::log(T(2));
}

template<class T>
T logistic(const T& number)
{
	return 1 / (1 + std::exp(-number));
}

template<class T>
bool isNaN(const T& value)
{
	return value != value; // this is true for NaN only
}

template<class T>
T round(const T& number)
{
	return (number > 0.0) ? floor(number + 0.5) : ceil(number - 0.5);
}

template<class T>
T roundToOdd(const T& number)
{
	return round((number + 1.0) / 2.0) * 2.0 - 1.0;
}

template<class T>
T clamp(const T& value, const T& minimum, const T& maximum)
{
	if (minimum > maximum) {
		throw std::logic_error("minimum > maximum in clamp()");
	}

	return std::min(std::max(value, minimum), maximum);
}

template<class Iterator>
typename Iterator::value_type mean(Iterator begin, Iterator end)
{
	size_t count = std::distance(begin, end);
	if (count == 0) {
		throw std::logic_error("empty range in mean()");
	}
	return static_cast<typename Iterator::value_type>(std::accumulate(begin, end, 0.0) / count);
}

template<class Container>
typename Container::value_type mean(Container container)
{
	return mean(container.begin(), container.end());
}

template<class Iterator>
typename Iterator::value_type stddev(Iterator begin, Iterator end)
{
	size_t count = std::distance(begin, end);
	if (count < 2) {
		throw std::logic_error("stddev() requires at least 2 elements");
	}
	typename Iterator::value_type myMean = mean(begin, end);
	double sumOfSquares = 0;
	while (begin != end) {
		sumOfSquares += (*begin - myMean) * (*begin - myMean);
		++begin;
	}
	return static_cast<typename Iterator::value_type>(std::sqrt(sumOfSquares / (count - 1)));	// -1 because it's a sample
}

template<class Container>
typename Container::value_type stddev(Container container)
{
	return stddev(container.begin(), container.end());
}


template<class Iterator>
typename Iterator::value_type sem(Iterator begin, Iterator end)
{
	size_t count = std::distance(begin, end);
	if (count < 2) {
		throw std::logic_error("sem() requires at least 2 elements");
	}
	return static_cast<typename Iterator::value_type>(stddev(begin, end) / std::sqrt(static_cast<double>(count)));
}

template<class Container>
typename Container::value_type sem(Container container)
{
	return sem(container.begin(), container.end());
}

template<class Iterator>
typename Iterator::value_type entropy(Iterator begin, Iterator end)
{
	typedef typename Iterator::value_type T;
	size_t count = std::distance(begin, end);
	if (count < 1) {
		throw std::logic_error("entropy() requires at least 1 element");
	}
	Iterator min = std::min_element(begin, end);
	if (*min < 0) {
		throw std::logic_error("entropy() is not defined for negative input values");
	}
	Iterator max = std::max_element(begin, end);
	if (*max == 0) {
		throw std::logic_error("entropy() is not defined for all-zero input values");
	}
	T tentativeEntropy(0);
	for (Iterator iter = begin; iter != end; ++iter) {
		if (*iter) {	// 0 would produce NaN below
			tentativeEntropy -= *iter * log2(*iter);
		}
	}
	// correct the entropy in case the pmf was not normalized
	T sum = std::accumulate(begin, end, T(0));
	return log2(sum) + tentativeEntropy / sum;
}

template<class Container>
typename Container::value_type entropy(Container container)
{
	return entropy(container.begin(), container.end());
}

template<class T>
double maxEntropy(T sampleSpaceSize)
{
	return log2(static_cast<double>(sampleSpaceSize));
}

template<class T>
class SparseHistogram {
private:
	class Bin {
	public:
		Bin(T begin, T end) :
			begin(begin),
			end(end),
			count()
		{
		}

		bool operator<(const Bin& other)
		{
			return begin < other.begin;
		}

		Bin& operator++()
		{
			++count;
		}

	private:
		T begin;
		T end;
		size_t count;
	};

public:
	SparseHistogram(T binSize) :
		binSize(binSize)
	{
		assert(binSize > 0);
	}

	Bin& operator[](T value)
	{
		return bins.insert(Bin(getBeginFor(value), getBeginFor(value) + binSize)).first;	// only inserted if it doesn't exist
	}

	Bin& getMode()
	{
		if (bins.empty()) {
			throw std::logic_error("mode of empty histogram is undefined");
		}

		typename std::set<Bin>::iterator best = bins.begin();
		for (typename std::set<Bin>::iterator iter = ++bins.begin(); iter != bins.end(); ++iter) {
			if (iter->count > best->count) {
				best = iter;
			}
		}
		return *best;
	}

private:
	T getBeginFor(T value)
	{
		return std::floor(value / binSize) * binSize;
	}

	T binSize;
	std::set<Bin> bins;
};

template<class T>
class DenseHistogram {
public:
	DenseHistogram(T lowerBound, T binSize, size_t binCount) :
		lowerBound(lowerBound),
		binSize(binSize),
		binCount(binCount),
		bins(binCount)
	{
	}

private:
	T lowerBound;
	T binSize;
	size_t binCount;
	std::vector<size_t> bins;
};

#endif
