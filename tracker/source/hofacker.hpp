#ifndef hofacker_hpp
#define hofacker_hpp

/*
Dynamic programming routine that optimizes total probability for occlusion and sequence observations.
Total probability may be improved by a "flip" operation, that inverts (multiply by -1) scores for two occlusions and all sequence scores between those occlusions.

parameters:
  sScores: scores for sequences (as probabilities)
  tScores: scores for occlusions (as probabilities)

returns:
  vector<bool>: true at flip positions, false otherwise
*/

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <limits>

template<class T>
std::vector<bool> hofacker(const std::vector<T>& sScores, const std::vector<T>& tScores)
{
	if (sScores.size() + 1 != tScores.size()) {
		throw std::domain_error("hofacker: sScores.size() + 1 must be tScores.size()");
	}

	std::vector<T> s(1);	// initialize with leading 0
	s.insert(s.end(), sScores.begin(), sScores.end());
	std::vector<T> t(1);	// initialize with leading 0
	t.insert(t.end(), tScores.begin(), tScores.end());
	
	const size_t n = s.size();

	std::vector<T> sNeg(n);	// sNeg(i) holds the best score for the subsequence 0...i where the sign at i is negative
	std::vector<T> sPos(n);	// sPos(i) holds the best score for the subsequence 0...i where the sign at i is positive
	std::vector<T> tNeg(n + 1);
	std::vector<T> tPos(n + 1);

	tNeg[0] = -std::numeric_limits<T>::infinity();

	for (size_t i = 0; i != n; ++i) {
		sNeg[i] = tNeg[i] - s[i];
		sPos[i] = tPos[i] + s[i];
		tNeg[i + 1] = std::max(sNeg[i] + t[i + 1], sPos[i] - t[i + 1]);
		tPos[i + 1] = std::max(sPos[i] + t[i + 1], sNeg[i] - t[i + 1]);
	}

	std::vector<bool> flip(n + 1);
	bool currentlyPositive = tPos.back() > tNeg.back();
	for (size_t i = n; i != 0; --i) {
		if (currentlyPositive) {
			T smallIfNoFlip = std::abs(tPos[i] - (sPos[i - 1] + t[i]));
			T smallIfFlip = std::abs(tPos[i] - (sNeg[i - 1] - t[i]));
			if (smallIfFlip < smallIfNoFlip) {
				flip[i] = true;
				currentlyPositive = false;
			}
		} else {
			T smallIfNoFlip = std::abs(tNeg[i] - (sNeg[i - 1] + t[i]));
			T smallIfFlip = std::abs(tNeg[i] - (sPos[i - 1] - t[i]));
			if (smallIfFlip < smallIfNoFlip) {
				flip[i] = true;
				currentlyPositive = true;
			}
		}
	}

	return std::vector<bool>(flip.begin() + 1, flip.end());
}

#endif
