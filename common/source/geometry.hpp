#ifndef geometry_hpp
#define geometry_hpp

#include <cmath>
#include <stdexcept>

// takes two angles in [0,360) and returns the difference in (-180,180) measured from "first" to "second"
template<class T>
T angleDifference(const T& first, const T& second)
{
	if (std::abs(second - first) <= 180) {
		return second - first;
	} else {
		if (second < first) {
			return second + 360 - first;
		} else {
			return second - 360 - first;
		}
	}
}

template<class T>
T eccentricity(T majorAxisLength, T minorAxisLength)
{
	if (majorAxisLength < minorAxisLength) {
		throw std::logic_error("major axis must not be shorter than minor axis");
	}

	if (majorAxisLength < 0 || minorAxisLength < 0) {
		throw std::logic_error("axis length cannot be negative");
	}

	if (majorAxisLength == 0) {
		// a dot has 0 eccentricity
		return 0;
	}

	T axisRatio = minorAxisLength / majorAxisLength;
	return std::sqrt(1 - axisRatio * axisRatio);
}

#endif
