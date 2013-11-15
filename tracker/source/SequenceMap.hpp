#ifndef SequenceMap_hpp
#define SequenceMap_hpp

#include <vector>
#include "OcclusionMap.hpp"
#include "FlyAttributes.hpp"
#include "../../common/source/MyBool.hpp"

class SequenceMap {
public:
	SequenceMap(const OcclusionMap& occlusionMap, const std::vector<float>& firstFlySizes, const std::vector<float>& secondFlySizes, const std::vector<MyBool>& isMissegmented, float sSizeWeight, bool discardMissegmentations);
	size_t size() const;

	std::vector<size_t> begin;	// STL-like index of the first frame of the occlusion
	std::vector<size_t> end;	// STL-like index of the first frame after (!) the occlusion

	// s-scores, s-probabilities and s-logodds
	std::vector<float> sSizeProb;

	std::vector<float> sSize;	// positive when the first fly is larger

	std::vector<float> sCombined;

private:
};

#endif
