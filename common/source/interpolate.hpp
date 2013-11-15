#ifndef interpolate_hpp
#define interpolate_hpp

/*
Reads the values *first and *last and linearly interpolates between them.
*first is written, but *last is not.

For interpolateDegrees we assume the values are in [0,360) and so are the values written.
*/

#include <algorithm>

template<class Iterator>
void interpolate(Iterator first, Iterator last, typename Iterator::value_type firstValue, typename Iterator::value_type lastValue)
{
	typename Iterator::value_type differenceValue = lastValue - firstValue;
	typename Iterator::difference_type distance = std::distance(first, last);
	
	size_t offset = 0;
	while (first != last) {
		*first = firstValue + differenceValue * (double(offset) / double(distance));
		++offset;
		++first;
	}
}

template<class Iterator>
void interpolate(Iterator first, Iterator last)
{
	interpolate(first, last, *first, *last);
}

template<class Iterator>
void interpolateRelative(Iterator first, Iterator last, Iterator relativeTo)
{
	typename Iterator::value_type firstValue = *first - *relativeTo;
	typename Iterator::value_type lastValue = *last - *(relativeTo + (last - first));
	typename Iterator::value_type differenceValue = lastValue - firstValue;
	typename Iterator::difference_type distance = std::distance(first, last);
	
	size_t offset = 0;
	while (first != last) {
		*first = *relativeTo + firstValue + differenceValue * (double(offset) / double(distance));
		++offset;
		++first;
		++relativeTo;
	}
}

template<class Iterator>
void interpolate_wrap(Iterator first, Iterator last, typename Iterator::value_type firstValue, typename Iterator::value_type lastValue, typename Iterator::value_type wrap)
{
	typename Iterator::value_type differenceValue = lastValue - firstValue;
	if (std::abs(differenceValue) * 2 > wrap) {
		// shortest interpolation is through 0
		if (firstValue > lastValue) {
			differenceValue = lastValue - (firstValue - wrap);
		} else {
			differenceValue = (lastValue - wrap) - firstValue;
		}
	}
	
	typename Iterator::difference_type distance = std::distance(first, last);
	
	size_t offset = 0;
	while (first != last) {
		*first = firstValue + differenceValue * (double(offset) / double(distance));
		if (*first < 0) {
			*first += wrap;
		}
		if (*first >= wrap) {
			*first -= wrap;
		}
		++offset;
		++first;
	}
}

template<class Iterator>
void interpolate_wrap(Iterator first, Iterator last, typename Iterator::value_type wrap)
{
	interpolate_wrap(first, last, *first, *last, wrap);
}

#endif
