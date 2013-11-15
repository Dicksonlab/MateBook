#ifndef score2prob_hpp
#define score2prob_hpp

template<class T>
T score2prob(T score)
{
	return score / 2 + T(0.5);
}

#endif
