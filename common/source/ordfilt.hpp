#ifndef ordfilt_hpp
#define ordfilt_hpp

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <cassert>

// neighborhood contains offsets from the current element that are to be considered
template<class T>
std::vector<T> ordfilt(const std::vector<T>& signal, double quantile, const std::vector<ptrdiff_t>& neighborhood)
{
	if (quantile < 0 || quantile > 1) {
		throw std::domain_error("quantile must be in [0,1]");
	}
	
	std::vector<T> ret;
	ret.reserve(signal.size());
	
	std::vector<T> neighborhoodValues;
	neighborhoodValues.reserve(neighborhood.size());
	
	for (size_t i = 0; i != signal.size(); ++i) {
		neighborhoodValues.clear();
		
		for (std::vector<ptrdiff_t>::const_iterator neighborhoodIter = neighborhood.begin(); neighborhoodIter != neighborhood.end(); ++neighborhoodIter) {
			ptrdiff_t offset = *neighborhoodIter;
			if (i + offset >= 0 && i + offset < signal.size()) {
				neighborhoodValues.push_back(signal[i + offset]);
			}
		}
		
		if (neighborhoodValues.empty()) {
			ret.push_back(std::numeric_limits<T>::quiet_NaN());
			continue;
		}
		
		size_t order = static_cast<size_t>(quantile * neighborhoodValues.size());
		if (order == neighborhoodValues.size()) {	// extreme case where quantile was 1.0
			--order;
		}
		
		typename std::vector<T>::iterator resultIter = neighborhoodValues.begin() + order;
		std::nth_element(neighborhoodValues.begin(), resultIter, neighborhoodValues.end());
		ret.push_back(*resultIter);
	}
	return ret;
}

// constructs a centered neighborhood with width elements and calls the function above
template<class T>
std::vector<T> ordfilt(const std::vector<T>& signal, double quantile, size_t width)
{
	ptrdiff_t signedWidth = static_cast<ptrdiff_t>(width);
	std::vector<ptrdiff_t> neighborhood;
	neighborhood.reserve(width);
	for (ptrdiff_t offset = -(signedWidth / 2); offset <= (signedWidth / 2); ++offset) {
		neighborhood.push_back(offset);
	}
	return ordfilt(signal, quantile, neighborhood);
}

// neighborhood contains offsets from the current element that are to be considered
template<class T>
std::vector<T> ordfilt_mirror(const std::vector<T>& signal, double quantile, const std::vector<ptrdiff_t>& neighborhood)
{
	if (quantile < 0 || quantile > 1) {
		throw std::domain_error("quantile must be in [0,1]");
	}

	std::vector<T> ret;
	ret.reserve(signal.size());

	std::vector<T> neighborhoodValues;
	neighborhoodValues.reserve(neighborhood.size());

	ptrdiff_t signalSize = signal.size();	// so we're using a signed type
	for (ptrdiff_t i = 0; i != signalSize; ++i) {
		neighborhoodValues.clear();

		for (std::vector<ptrdiff_t>::const_iterator neighborhoodIter = neighborhood.begin(); neighborhoodIter != neighborhood.end(); ++neighborhoodIter) {
			ptrdiff_t offset = *neighborhoodIter;
			ptrdiff_t signalIndex = i + offset;

			if (signalIndex < 0) {
				if (std::abs(static_cast<int>(signalIndex / signalSize)) % 2 == 0) {	//TODO: remove the cast: it is a workaround for Win64
					// use mirrored signal
					signalIndex = std::abs(static_cast<int>(signalIndex)) % signalSize;	//TODO: remove the cast: it is a workaround for Win64
				} else {
					// use non-mirrored signal
					signalIndex = signalIndex % signalSize + signalSize;
				}
			} else {
				if ((signalIndex / signalSize) % 2 == 1) {
					// use mirrored signal
					signalIndex = signalSize - 1 - (signalIndex % signalSize);
				} else {
					// use non-mirrored signal
					signalIndex = signalIndex % signalSize;
				}
			}

			assert(signalIndex >= 0 && signalIndex < signalSize);
			neighborhoodValues.push_back(signal[signalIndex]);
		}

		if (neighborhoodValues.empty()) {
			ret.push_back(std::numeric_limits<T>::quiet_NaN());
			continue;
		}

		size_t order = quantile * neighborhoodValues.size();
		if (order == neighborhoodValues.size()) {	// extreme case where quantile was 1.0
			--order;
		}

		typename std::vector<T>::iterator resultIter = neighborhoodValues.begin() + order;
		std::nth_element(neighborhoodValues.begin(), resultIter, neighborhoodValues.end());
		ret.push_back(*resultIter);
	}
	return ret;
}

// neighborhood contains offsets from the current element that are to be considered
template<class T>
std::vector<T> ordfilt_wrap(const std::vector<T>& signal, double quantile, const std::vector<ptrdiff_t>& neighborhood)
{
	if (quantile < 0 || quantile > 1) {
		throw std::domain_error("quantile must be in [0,1]");
	}

	std::vector<T> ret;
	ret.reserve(signal.size());

	std::vector<T> neighborhoodValues;
	neighborhoodValues.reserve(neighborhood.size());

	for (ptrdiff_t i = 0; i != signal.size(); ++i) {
		neighborhoodValues.clear();

		for (std::vector<ptrdiff_t>::const_iterator neighborhoodIter = neighborhood.begin(); neighborhoodIter != neighborhood.end(); ++neighborhoodIter) {
			ptrdiff_t offset = *neighborhoodIter;
			ptrdiff_t signalIndex = (i + offset) % static_cast<ptrdiff_t>(signal.size());
			if (signalIndex < 0) {
				signalIndex += static_cast<ptrdiff_t>(signal.size());
			}
			assert(signalIndex >= 0 && signalIndex < signal.size());
			neighborhoodValues.push_back(signal[signalIndex]);
		}

		if (neighborhoodValues.empty()) {
			ret.push_back(std::numeric_limits<T>::quiet_NaN());
			continue;
		}

		size_t order = quantile * neighborhoodValues.size();
		if (order == neighborhoodValues.size()) {	// extreme case where quantile was 1.0
			--order;
		}

		typename std::vector<T>::iterator resultIter = neighborhoodValues.begin() + order;
		std::nth_element(neighborhoodValues.begin(), resultIter, neighborhoodValues.end());
		ret.push_back(*resultIter);
	}
	return ret;
}

#endif
