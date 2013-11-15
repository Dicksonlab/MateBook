#ifndef prob2logodd_hpp
#define prob2logodd_hpp

#include <cmath>

template<class T>
T prob2logodd(T prob)
{
	T cap = 20;
	prob = std::max<T>(0, prob);
	prob = std::min<T>(1, prob);
	return std::max<T>(-cap, std::min<T>(cap, std::log(prob / (1 - prob))));
}

#endif
