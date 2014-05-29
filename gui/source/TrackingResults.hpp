#ifndef TrackingResults_hpp
#define TrackingResults_hpp

#include <map>
#include <string>
#include <vector>
#include "OcclusionMap.hpp"
#include "../../tracker/source/FrameAttributes.hpp"
#include "../../tracker/source/FlyAttributes.hpp"
#include "../../tracker/source/PairAttributes.hpp"

#include <memory>

class TrackingResults {
public:
	TrackingResults(const std::string& fileName, const std::string& contourFileName, const std::string& smoothHistogramFileName);

	~TrackingResults();

	std::string getFileName() const;

	size_t getFlyCount() const;
	bool hasData(size_t videoFrameNumber) const;

	std::vector<std::string> getFrameAttributeNames() const;
	std::vector<std::string> getFlyAttributeNames() const;
	std::vector<std::string> getPairAttributeNames() const;

	template<class T>
	bool hasFrameAttribute(std::string attributeName) const
	{
		return frameAttributes.has<T>(attributeName);
	}

	template<class T>
	bool hasFlyAttribute(std::string attributeName) const
	{
		if (flyAttributes.empty()) {
			return false;
		}
		return flyAttributes[0].has<T>(attributeName);
	}

	template<class T>
	bool hasPairAttribute(std::string attributeName) const
	{
		if (pairAttributes.empty()) {
			return false;
		}
		return pairAttributes[0][0].has<T>(attributeName);
	}

	template<class T>
	Attribute<T> getFrameData(std::string attributeName) const
	{
		if (frameAttributes.isOcclusionSScore(attributeName)) {
			const std::vector<bool>& switchMask = getOcclusionMap().getSwitchMask();
			Attribute<T> ret(frameAttributes.getFilled<T>(attributeName));
			assert(switchMask.size() == ret.size());
			for (size_t frameNumber = 0; frameNumber != ret.size(); ++frameNumber) {
				if (switchMask[frameNumber]) {
					ret[frameNumber] = -ret[frameNumber];
				}
			}
			return ret;
		} else if (frameAttributes.isOcclusionTScore(attributeName)) {
			Attribute<T> ret(frameAttributes.getFilled<T>(attributeName));
			for (size_t frameNumber = 0; frameNumber != ret.size(); ++frameNumber) {
				if (getOcclusionMap().getCurrentOcclusion(frameNumber + getOffset()).isSwitched()) {
					ret[frameNumber] = -ret[frameNumber];
				}
			}
			return ret;
		} else if (frameAttributes.isOcclusionSProb(attributeName)) {
			const std::vector<bool>& switchMask = getOcclusionMap().getSwitchMask();
			Attribute<T> ret(frameAttributes.getFilled<T>(attributeName));
			assert(switchMask.size() == ret.size());
			for (size_t frameNumber = 0; frameNumber != ret.size(); ++frameNumber) {
				if (switchMask[frameNumber]) {
					ret[frameNumber] = T(1) - ret[frameNumber];
				}
			}
			return ret;
		} else if (frameAttributes.isOcclusionTProb(attributeName)) {
			Attribute<T> ret(frameAttributes.getFilled<T>(attributeName));
			for (size_t frameNumber = 0; frameNumber != ret.size(); ++frameNumber) {
				if (getOcclusionMap().getCurrentOcclusion(frameNumber + getOffset()).isSwitched()) {
					ret[frameNumber] = T(1) - ret[frameNumber];
				}
			}
			return ret;
		}
		return frameAttributes.getFilled<T>(attributeName);
	}

	template<class T>
	T getFrameData(std::string attributeName, size_t videoFrameNumber) const
	{
		return getFrameData<T>(attributeName)[videoFrameNumber - getOffset()];
	}

	template<class T>
	void setFrameData(std::string attributeName, size_t videoFrameNumber, const T& value)
	{
		frameAttributes.getFilled<T>(attributeName)[videoFrameNumber - getOffset()] = value;
	}

	template<class T>
	Attribute<T> getFlyData(size_t flyNumber, std::string attributeName) const
	{
		assert(flyNumber < flyCount);
		if (flyCount == 1) {
			return flyAttributes[0].getFilled<T>(attributeName);
		} else if (flyCount == 2) {
			const std::vector<bool>& switchMask = getOcclusionMap().getSwitchMask();
			Attribute<T> ret = flyAttributes[0].getFilled<T>(attributeName);	// initialized with fly0's data
			const Attribute<T>& retFly1 = flyAttributes[1].getFilled<T>(attributeName);
			const Attribute<MyBool>& isOcclusion = getFrameData<MyBool>("isOcclusion");
			assert(ret.size() == switchMask.size() && retFly1.size() == switchMask.size() && isOcclusion.size() == switchMask.size());
			for (size_t frameNumber = 0; frameNumber != switchMask.size(); ++frameNumber) {
				if (isOcclusion[frameNumber] && getOcclusionMap().getCurrentOcclusion(frameNumber + getOffset()).isSwitched()) {
					ret[frameNumber] = T();
				} else if (flyNumber != switchMask[frameNumber]) {
					ret[frameNumber] = retFly1[frameNumber];
				}
			}
			return ret;
		}
		assert(false);	//TODO: make this work for an arbitrary number of flies	
		return Attribute<T>();
	}

	template<class T>
	T getFlyData(size_t flyNumber, std::string attributeName, size_t videoFrameNumber) const
	{
		bool switchFlies = getOcclusionMap().getSwitchMask()[videoFrameNumber - getOffset()];
		if (getFrameData<MyBool>("isOcclusion", videoFrameNumber) && getOcclusionMap().getCurrentOcclusion(videoFrameNumber).isSwitched()) {
			return T();
		} else {
			return getRawFlyData<T>(flyNumber != switchFlies, attributeName)[videoFrameNumber - getOffset()];
		}
	}

	template<class T>
	Attribute<T> getPairData(size_t activeFly, size_t passiveFly, std::string attributeName) const
	{
		assert(flyCount == 2);
		assert(activeFly < flyCount && passiveFly < flyCount && activeFly != passiveFly);
		const std::vector<bool>& switchMask = getOcclusionMap().getSwitchMask();
		Attribute<T> ret = pairAttributes[0][1].getFilled<T>(attributeName);	// initialized with the [0][1] pair's data
		const Attribute<T>& retFly1 = pairAttributes[1][0].getFilled<T>(attributeName);
		const Attribute<MyBool>& isOcclusion = getFrameData<MyBool>("isOcclusion");
		assert(ret.size() == switchMask.size() && retFly1.size() == switchMask.size() && isOcclusion.size() == switchMask.size());
		for (size_t frameNumber = 0; frameNumber != switchMask.size(); ++frameNumber) {
			if (isOcclusion[frameNumber] && getOcclusionMap().getCurrentOcclusion(frameNumber + getOffset()).isSwitched()) {
				ret[frameNumber] = T();
			} else if (activeFly != switchMask[frameNumber]) {
				ret[frameNumber] = retFly1[frameNumber];
			}
		}
		return ret;
	}

	const OcclusionMap& getOcclusionMap() const;
	OcclusionMap& getOcclusionMap();

	const std::vector<char>& getContours() const;
	const float* getSmoothHistogram(size_t videoFrameNumber) const;

	size_t getOffset() const;	// number of frames at the beginning of the video that were skipped

	void saveAnnotation(const std::string& fileName) const;

private:
	// the getRaw* methods return the data without performing the switch operations
	// therefore they are much faster than their public get* couterparts
	template<class T>
	const std::vector<T>& getRawFrameData(std::string attributeName) const
	{
		return frameAttributes.getFilled<T>(attributeName).getData();
	}

	template<class T>
	T getRawFrameData(std::string attributeName, size_t videoFrameNumber) const
	{
		return getRawFrameData<T>(attributeName)[videoFrameNumber - getOffset()];
	}

	template<class T>
	const std::vector<T>& getRawFlyData(size_t flyNumber, std::string attributeName) const
	{
		assert(flyNumber < flyCount);
		return flyAttributes[flyNumber].getFilled<T>(attributeName).getData();
	}

	template<class T>
	T getRawFlyData(size_t flyNumber, std::string attributeName, size_t videoFrameNumber) const
	{
		return getRawFlyData<T>(flyNumber, attributeName)[videoFrameNumber - getOffset()];
	}

	// these get* methods return AbstractAttribute objects and are needed for saving annotations
	std::auto_ptr<AbstractAttribute> getFrameData(std::string attributeName) const;
	std::auto_ptr<AbstractAttribute> getFlyData(size_t flyNumber, std::string attributeName) const;
	std::auto_ptr<AbstractAttribute> getPairData(size_t activeFly, size_t passiveFly, std::string attributeName) const;

	std::string fileName;
	std::string contourFileName;
	std::string smoothHistogramFileName;
	size_t flyCount;

	FrameAttributes frameAttributes;
	std::vector<FlyAttributes> flyAttributes;	// [flyNumber]
	std::vector<std::vector<PairAttributes> > pairAttributes;	// [activeFly][passiveFly]

	OcclusionMap occlusionMap;

	std::vector<char> contours;
	std::vector<float> smoothHistograms;
};

#endif
