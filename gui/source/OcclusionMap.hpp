#ifndef OcclusionMap_hpp
#define OcclusionMap_hpp

#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "../../tracker/source/Attribute.hpp"

class Occlusion {
public:
	Occlusion(size_t number, size_t begin, size_t end);

	size_t getNumber() const;
	size_t getBegin() const;
	size_t getEnd() const;
	size_t frameCount() const;

	void doSwitch();
	bool isSwitched() const;

private:
	size_t number;
	size_t begin;
	size_t end;
	bool switched;
};

// for binary search
class OcclusionBeginComparator {
public:
	inline
	bool operator()(const Occlusion& left, const Occlusion& right) const
	{
		return left.getBegin() < right.getBegin();
	}
};

class OcclusionMap {
public:
	OcclusionMap();

	template<class T>
	OcclusionMap(const std::vector<T>& isOcclusion, size_t offset) :
		offset(offset),
		frameCount(isOcclusion.size()),
		switchMask(frameCount, false)
	{
		size_t occlusionNumber = 0;

		if (frameCount == 0 || isOcclusion[0] == false) {
			// add a 0-frame occlusion at the beginning
			occlusions.push_back(Occlusion(occlusionNumber++, offset, offset));
		}

		bool previousOccluded = false;
		size_t currentBegin = 0;
		for (size_t frameNumber = 0; frameNumber != isOcclusion.size(); ++frameNumber) {
			if (isOcclusion[frameNumber] && !previousOccluded) {
				previousOccluded = true;
				currentBegin = frameNumber;
			} else if (!isOcclusion[frameNumber] && previousOccluded) {
				previousOccluded = false;
				occlusions.push_back(Occlusion(occlusionNumber++, currentBegin + offset, frameNumber + offset));
			}
		}
		if (previousOccluded) {
			// the video ends in an occlusion
			occlusions.push_back(Occlusion(occlusionNumber++, currentBegin + offset, frameCount + offset));
		} else {
			// add a 0-frame occlusion at the end
			occlusions.push_back(Occlusion(occlusionNumber++, frameCount + offset, frameCount + offset));
		}
	}

	size_t size() const;

	bool hasOcclusion(size_t occlusionNumber) const;
	Occlusion getOcclusion(size_t occlusionNumber) const;
	Occlusion getCurrentOcclusion(size_t frameNumber) const;
	bool hasPreviousOcclusion(size_t frameNumber) const;
	Occlusion getPreviousOcclusion(size_t frameNumber) const;
	bool hasNextOcclusion(size_t frameNumber) const;
	Occlusion getNextOcclusion(size_t frameNumber) const;

	void switchBefore(size_t occlusionNumber);
	void switchLocally(size_t occlusionNumber, const Attribute<MyBool>& occlusionInspected, const Attribute<float>& sCombined, const Attribute<float>& tCombined);
	void switchAfter(size_t occlusionNumber);

	const std::vector<bool>& getSwitchMask() const;

private:
	void buildSwitchMask();

	std::vector<Occlusion> occlusions;
	size_t offset;
	size_t frameCount;
	std::vector<bool> switchMask;
};

#endif
