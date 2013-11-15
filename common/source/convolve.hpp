#ifndef convolve_hpp
#define convolve_hpp

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <limits>

// the result has the same length as signal
template<class T, class K>
std::vector<T> convolve(const std::vector<T>& signal, const std::vector<K>& kernel)
{
	ptrdiff_t kernelSize = static_cast<ptrdiff_t>(kernel.size());
	ptrdiff_t offset = kernelSize / 2;	//TODO: write an overload where the offset can be specified

	std::vector<T> ret;
	ret.resize(signal.size());

	for (ptrdiff_t i = 0; i != signal.size(); ++i) {
		const ptrdiff_t begin = std::max<ptrdiff_t>(0, i - offset);
		const ptrdiff_t end = std::min<ptrdiff_t>(signal.size(), i - offset + kernelSize);
		for (ptrdiff_t j = begin; j < end; ++j) {
			ret[i] += signal[j] * kernel[j - i + offset];
		}
	}
	return ret;
}

// the result has the same length as signal
// values outside the signal are assumed to be the same as the value on the signal's border
template<class T, class K>
std::vector<T> convolve_clamp(const std::vector<T>& signal, const std::vector<K>& kernel)
{
	// empty signals don't have a border, so we handle this special case first
	if (signal.empty()) {
		return signal;
	}

	const T leftBorder = signal.front();
	const T rightBorder = signal.back();

	ptrdiff_t kernelSize = static_cast<ptrdiff_t>(kernel.size());
	ptrdiff_t offset = kernelSize / 2;	//TODO: write an overload where the offset can be specified

	std::vector<T> ret;
	ret.resize(signal.size());

	for (ptrdiff_t i = 0; i != signal.size(); ++i) {
		for (ptrdiff_t j = i - offset; j < i - offset + kernelSize; ++j) {
			if (j < 0) {
				ret[i] += leftBorder * kernel[j - i + offset];
			} else if (j >= signal.size()) {
				ret[i] += rightBorder * kernel[j - i + offset];
			} else {
				ret[i] += signal[j] * kernel[j - i + offset];
			}
		}
	}
	return ret;
}

#endif
