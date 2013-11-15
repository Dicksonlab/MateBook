#include "OcclusionMap.hpp"
#include <limits>

Occlusion::Occlusion(size_t number, size_t begin, size_t end) :
	number(number),
	begin(begin),
	end(end),
	switched(false)
{
}

size_t Occlusion::getNumber() const
{
	return number;
}

size_t Occlusion::getBegin() const
{
	return begin;
}

size_t Occlusion::getEnd() const
{
	return end;
}

size_t Occlusion::frameCount() const
{
	return end - begin;
}

void Occlusion::doSwitch()
{
	switched = !switched;
}

bool Occlusion::isSwitched() const
{
	return switched;
}

OcclusionMap::OcclusionMap() :
	offset(0),
	frameCount(0)
{
}

size_t OcclusionMap::size() const
{
	return occlusions.size();
}

bool OcclusionMap::hasOcclusion(size_t occlusionNumber) const
{
	return occlusionNumber < occlusions.size();
}

Occlusion OcclusionMap::getOcclusion(size_t occlusionNumber) const
{
	if (occlusionNumber >= occlusions.size()) {
		throw std::logic_error("getOcclusion: no more occlusions");
	}
	return occlusions[occlusionNumber];
}

Occlusion OcclusionMap::getCurrentOcclusion(size_t frameNumber) const
{
	std::vector<Occlusion>::const_iterator greater = std::upper_bound(occlusions.begin(), occlusions.end(), Occlusion(0, frameNumber, 0), OcclusionBeginComparator());
	if (greater != occlusions.begin()) {
		return *(--greater);
	}
	if (greater != occlusions.end()) {
		return *greater;
	}
	throw std::logic_error("getCurrentOcclusion not possible on empty OcclusionMap");
}

bool OcclusionMap::hasPreviousOcclusion(size_t frameNumber) const
{
	return std::upper_bound(occlusions.begin(), occlusions.end(), Occlusion(0, frameNumber, 0), OcclusionBeginComparator()) != occlusions.begin();
}

Occlusion OcclusionMap::getPreviousOcclusion(size_t frameNumber) const
{
	std::vector<Occlusion>::const_iterator greater = std::upper_bound(occlusions.begin(), occlusions.end(), Occlusion(0, frameNumber, 0), OcclusionBeginComparator());
	if (greater == occlusions.begin()) {
		throw std::logic_error("getPreviousOcclusion: no more occlusions");
	}
	return *(--greater);
}

bool OcclusionMap::hasNextOcclusion(size_t frameNumber) const
{
	return std::upper_bound(occlusions.begin(), occlusions.end(), Occlusion(0, frameNumber, 0), OcclusionBeginComparator()) != occlusions.end();
}

Occlusion OcclusionMap::getNextOcclusion(size_t frameNumber) const
{
	std::vector<Occlusion>::const_iterator greater = std::upper_bound(occlusions.begin(), occlusions.end(), Occlusion(0, frameNumber, 0), OcclusionBeginComparator());
	if (greater == occlusions.end()) {
		throw std::logic_error("getNextOcclusion: no more occlusions");
	}
	return *greater;
}

void OcclusionMap::switchBefore(size_t occlusionNumber)
{
	occlusions[0].doSwitch();
	occlusions[occlusionNumber].doSwitch();
	buildSwitchMask();
}

void OcclusionMap::switchLocally(size_t occlusionNumber, const Attribute<MyBool>& occlusionInspected, const Attribute<float>& sCombined, const Attribute<float>& tCombined)
{
	assert(sCombined.size() == tCombined.size());

	if (sCombined.size() == 0) {
		return;
	}

	// find the pair of occlusions that contains occlusionNumber and improves the score as much as possible
	size_t bestPairedOcclusion = 0;
	float bestImprovement = -std::numeric_limits<float>::infinity();

	if (occlusionNumber != 0) {
		// look to the left
		size_t loopOcclusionNumber = occlusionNumber - 1;
		float scoreDelta = 0;
		do {
			scoreDelta -= sCombined[occlusions[loopOcclusionNumber].getEnd() - offset];
			float improvement = scoreDelta - (occlusions[loopOcclusionNumber].frameCount() ? tCombined[occlusions[loopOcclusionNumber].getBegin() - offset] : 0.0f);
			if (improvement > bestImprovement && (occlusions[loopOcclusionNumber].frameCount() == 0 || !occlusionInspected[occlusions[loopOcclusionNumber].getBegin() - offset])) {
				bestImprovement = improvement;
				bestPairedOcclusion = loopOcclusionNumber;
			}
		} while (loopOcclusionNumber--);
	}
	if (occlusionNumber + 1 < occlusions.size()) {
		// look to the right
		size_t loopOcclusionNumber = occlusionNumber + 1;
		float scoreDelta = 0;
		do {
			scoreDelta -= sCombined[occlusions[loopOcclusionNumber].getBegin() - 1 - offset];
			float improvement = scoreDelta - (occlusions[loopOcclusionNumber].frameCount() ? tCombined[occlusions[loopOcclusionNumber].getBegin() - offset] : 0.0f);
			if (improvement > bestImprovement && (occlusions[loopOcclusionNumber].frameCount() == 0 || !occlusionInspected[occlusions[loopOcclusionNumber].getBegin() - offset])) {
				bestImprovement = improvement;
				bestPairedOcclusion = loopOcclusionNumber;
			}
		} while (++loopOcclusionNumber != occlusions.size());
	}
	if (bestImprovement > -std::numeric_limits<float>::infinity()) {
		occlusions[bestPairedOcclusion].doSwitch();
		occlusions[occlusionNumber].doSwitch();
		buildSwitchMask();
	}
}

void OcclusionMap::switchAfter(size_t occlusionNumber)
{
	occlusions[occlusionNumber].doSwitch();
	buildSwitchMask();
}

const std::vector<bool>& OcclusionMap::getSwitchMask() const
{
	return switchMask;
}

void OcclusionMap::buildSwitchMask()
{
	std::fill(switchMask.begin(), switchMask.end(), false);
	bool switched = false;
	for (std::vector<Occlusion>::const_iterator iter = occlusions.begin(); iter != occlusions.end(); ++iter) {
		// build switchMask for this occlusion
		if (iter->isSwitched()) {
			switched = !switched;
		}
		for (size_t frameNumber = iter->getBegin(); frameNumber != iter->getEnd(); ++frameNumber) {
			switchMask[frameNumber - offset] = switched;
		}
		// build switchMask for the sequence directly after this occlusion
		if (switched) {
			if (iter + 1 == occlusions.end()) {
				// last occlusion
				for (size_t frameNumber = iter->getEnd(); frameNumber != frameCount + offset; ++frameNumber) {
					switchMask[frameNumber - offset] = true;
				}
			} else {
				for (size_t frameNumber = iter->getEnd(); frameNumber != (iter + 1)->getBegin(); ++frameNumber) {
					switchMask[frameNumber - offset] = true;
				}
			}
		}
	}
}
