#ifndef gaussian_hpp
#define gaussian_hpp

#include <vector>
#include <cmath>
#include "mathematics.hpp"

template<class T>
T gaussian(T mean, T stdDev, T arg)
{
	T transform = (arg - mean) / stdDev;
	return std::exp(-0.5 * transform * transform) / (stdDev * std::sqrt(2 * constant::PI));
}

template<class T>
std::vector<T> discreteGaussian(size_t size)
{
	std::vector<T> ret;
	ret.reserve(size);
	float mean = (static_cast<float>(size) - 1) / 2;
	float stdDev = static_cast<float>(size) / 6;

	float weightSum = 0;
	for (size_t i = 0; i != size; ++i) {
		ret.push_back(gaussian<float>(mean, stdDev, i));
		weightSum += ret.back();
	}
	for (size_t i = 0; i != size; ++i) {
		ret[i] /= weightSum;
	}
	return ret;
}

#endif
