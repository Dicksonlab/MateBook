#ifndef TrackedFrame_hpp
#define TrackedFrame_hpp

#include "Fly.hpp"
#include <vector>
#include <stdint.h>
#include "../../common/source/MyBool.hpp"
#include "../../common/source/Vec.hpp"

class TrackedFrame {	//TODO: rename to SegmentedFrame
public:
	TrackedFrame(
		cv::Size frameSize,
		float sourceFrameRate,
		size_t videoFrame,
		size_t trackedFrame,
		float videoFrameRelative,
		float trackedFrameRelative,
		const std::vector<Fly>& flies,
		size_t knownFlyCount,
		bool missegmented,
		unsigned char bodyThreshold,
		unsigned char wingThreshold,
		size_t bodyContourOffset,
		size_t wingContourOffset
	);

	float get_videoTime() const;
	float get_trackedTime() const;
	uint32_t get_videoFrame() const;
	uint32_t get_trackedFrame() const;
	float get_videoFrameRelative() const;
	float get_trackedFrameRelative() const;

	const Fly& fly(size_t i) const;
	size_t flyCount() const;
	size_t getKnownFlyCount() const;

	MyBool get_isOcclusionTouched() const;
	MyBool get_isMissegmented() const;
	uint32_t get_bodyThreshold() const;
	uint32_t get_wingThreshold() const;
	uint32_t get_bodyContourOffset() const;
	uint32_t get_wingContourOffset() const;
	Vf2 get_averageBodyCentroid() const;
	float get_tBocScore() const;

	void rearrangeFlies(const TrackedFrame& lastFrame);
	void eraseContours();

private:
	cv::Size frameSize;
	float sourceFrameRate;
	size_t videoFrame;
	size_t trackedFrame;
	float videoFrameRelative;
	float trackedFrameRelative;
	std::vector<Fly> flies;
	size_t knownFlyCount;
	bool missegmented;
	bool isOcclusionTouched;
	unsigned char bodyThreshold;
	unsigned char wingThreshold;
	size_t bodyContourOffset;
	size_t wingContourOffset;

public:	//TODO: make this private
	// boc-related
	std::vector<cv::Point> carry0;
	std::vector<cv::Point> carry1;
	float bocScore;
};

#endif
