#include "SequenceMap.hpp"
#include <stdexcept>
#include "signTest.hpp"
#include "prob2logodd.hpp"

SequenceMap::SequenceMap(const OcclusionMap& occlusionMap, const std::vector<float>& firstFlySizes, const std::vector<float>& secondFlySizes, const std::vector<MyBool>& isMissegmented, float sSizeWeight, bool discardMissegmentations)
{
	if (firstFlySizes.size() != secondFlySizes.size()) {
		throw std::logic_error("flySizes vectors passed to SequenceMap constructor must have the same number of elements");
	}
	
	size_t frameCount = firstFlySizes.size();
	if (occlusionMap.size() == 0) {
		if (frameCount == 0) {
			return;
		}
		begin.push_back(0);
		end.push_back(frameCount);
		float p = 0;
		if (discardMissegmentations) {
			p = signTest(firstFlySizes.begin(), firstFlySizes.end(), secondFlySizes.begin(), isMissegmented.begin());
		} else {
			p = signTest(firstFlySizes.begin(), firstFlySizes.end(), secondFlySizes.begin());
		}
		sSizeProb.push_back(p);
		sSize.push_back(prob2logodd(sSizeProb.back()));
		sCombined.push_back(
			sSize.back() * sSizeWeight
		);
		return;
	}
	
	// if it's starting with a sequence (not an occlusion)
	if (occlusionMap.begin.front() != 0) {
		begin.push_back(0);
		end.push_back(occlusionMap.begin.front());
		float p = 0;
		if (discardMissegmentations) {
			p = signTest(firstFlySizes.begin(), firstFlySizes.begin() + end.back(), secondFlySizes.begin(), isMissegmented.begin());
		} else {
			p = signTest(firstFlySizes.begin(), firstFlySizes.begin() + end.back(), secondFlySizes.begin());
		}
		sSizeProb.push_back(p);
		sSize.push_back(prob2logodd(sSizeProb.back()));
		sCombined.push_back(
			sSize.back() * sSizeWeight
		);
	}
	
	// for the sequences after occlusions
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		if (occlusionNumber + 1 != occlusionMap.size()) {
			// there's another occlusion after this one
			begin.push_back(occlusionMap.end[occlusionNumber]);
			end.push_back(occlusionMap.begin[occlusionNumber + 1]);
			float p = 0;
			if (discardMissegmentations) {
				p = signTest(firstFlySizes.begin() + begin.back(), firstFlySizes.begin() + end.back(), secondFlySizes.begin() + begin.back(), isMissegmented.begin() + begin.back());
			} else {
				p = signTest(firstFlySizes.begin() + begin.back(), firstFlySizes.begin() + end.back(), secondFlySizes.begin() + begin.back());
			}
			sSizeProb.push_back(p);
			sSize.push_back(prob2logodd(sSizeProb.back()));
			sCombined.push_back(
				sSize.back() * sSizeWeight
			);
			continue;
		}
		
		// there's no other occlusion after this one; we only add another sequence if it's not ending in an occlusion
		if (occlusionMap.end[occlusionNumber] != frameCount) {
			begin.push_back(occlusionMap.end[occlusionNumber]);
			end.push_back(frameCount);
			float p = 0;
			if (discardMissegmentations) {
				p = signTest(firstFlySizes.begin() + begin.back(), firstFlySizes.begin() + end.back(), secondFlySizes.begin() + begin.back(), isMissegmented.begin() + begin.back());
			} else {
				p = signTest(firstFlySizes.begin() + begin.back(), firstFlySizes.begin() + end.back(), secondFlySizes.begin() + begin.back());
			}
			sSizeProb.push_back(p);
			sSize.push_back(prob2logodd(sSizeProb.back()));
			sCombined.push_back(
				sSize.back() * sSizeWeight
			);
		}
	}
}

size_t SequenceMap::size() const
{
	return begin.size();
}
