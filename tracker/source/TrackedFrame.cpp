#include "TrackedFrame.hpp"
#include <string>
#include <stdexcept>
#include "../../common/source/mathematics.hpp"
#include "hungarian.hpp"

TrackedFrame::TrackedFrame(
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
	) :
	frameSize(frameSize),
	sourceFrameRate(sourceFrameRate),
	videoFrame(videoFrame),
	trackedFrame(trackedFrame),
	videoFrameRelative(videoFrameRelative),
	trackedFrameRelative(trackedFrameRelative),
	flies(flies),
	knownFlyCount(knownFlyCount),
	missegmented(missegmented),
	isOcclusionTouched(flies.size() != knownFlyCount),
	bodyThreshold(bodyThreshold),
	wingThreshold(wingThreshold),
	bodyContourOffset(bodyContourOffset),
	wingContourOffset(wingContourOffset),
	bocScore()
{
	for (size_t flyNumber = 0; flyNumber != flies.size(); ++flyNumber) {
		if (flies[flyNumber].get_bodySplit()) {
			isOcclusionTouched = true;
			break;
		}
	}
}

float TrackedFrame::get_videoTime() const
{
	return get_videoFrame() / sourceFrameRate;
}

float TrackedFrame::get_trackedTime() const
{
	return get_trackedFrame() / sourceFrameRate;
}

uint32_t TrackedFrame::get_videoFrame() const
{
	return videoFrame;
}

uint32_t TrackedFrame::get_trackedFrame() const
{
	return trackedFrame;
}

float TrackedFrame::get_videoFrameRelative() const
{
	return videoFrameRelative;
}

float TrackedFrame::get_trackedFrameRelative() const
{
	return trackedFrameRelative;
}

const Fly& TrackedFrame::fly(size_t i) const
{
	return flies[i];
}

size_t TrackedFrame::flyCount() const
{
	return flies.size();
}

size_t TrackedFrame::getKnownFlyCount() const
{
	return knownFlyCount;
}

MyBool TrackedFrame::get_isOcclusionTouched() const
{
	return isOcclusionTouched;
}

MyBool TrackedFrame::get_isMissegmented() const
{
	return missegmented;
}

uint32_t TrackedFrame::get_bodyThreshold() const
{
	return bodyThreshold;
}

uint32_t TrackedFrame::get_wingThreshold() const
{
	return wingThreshold;
}

uint32_t TrackedFrame::get_bodyContourOffset() const
{
	return bodyContourOffset;
}

uint32_t TrackedFrame::get_wingContourOffset() const
{
	return wingContourOffset;
}

Vf2 TrackedFrame::get_averageBodyCentroid() const
{
	Vf2 centroid;

	if (flyCount() != 0) {	// note that for empty frames the averageBodyCentroid will stay 0,0
		for (size_t flyNumber = 0; flyNumber != flyCount(); ++flyNumber) {
			centroid += flies[flyNumber].get_bodyCentroidTracked();
		}
		centroid *= (1.0f / static_cast<float>(flyCount()));
	}

	return centroid;
}

float TrackedFrame::get_tBocScore() const
{
	return bocScore;
}

void TrackedFrame::rearrangeFlies(const TrackedFrame& lastFrame)
{
	if (getKnownFlyCount() != lastFrame.getKnownFlyCount()) {
		throw std::logic_error("cannot rearrange flies if the known fly counts do not match");
	}

	// we must not rearrange within or after occlusions! otherwise the boc-information might not match the body
	//TODO: what about missegmentations?
	if (get_isOcclusionTouched() || lastFrame.get_isOcclusionTouched()) {
		return;
	}

	// unless this or the last frame is an occlusion, line the flies up in the array so their identities match
	if (flyCount() == getKnownFlyCount() && lastFrame.flyCount() == lastFrame.getKnownFlyCount()) {
		cv::Mat distanceMatrix(lastFrame.flyCount(), flyCount(), CV_32F);
		for (size_t flyBefore = 0; flyBefore != lastFrame.flyCount(); ++flyBefore) {
			for (size_t flyNow = 0; flyNow != flyCount(); ++flyNow) {
				distanceMatrix.at<float>(flyBefore, flyNow) = (lastFrame.fly(flyBefore).get_bodyCentroidTracked() - flies[flyNow].get_bodyCentroidTracked()).norm();
			}
		}
		std::vector<size_t> beforeToNow = hungarian(distanceMatrix);
		std::vector<Fly> identifiedFliesInThisFrame;
		for (size_t flyIndex = 0; flyIndex != getKnownFlyCount(); ++flyIndex) {
			identifiedFliesInThisFrame.push_back(flies[beforeToNow[flyIndex]]);
		}
		flies = identifiedFliesInThisFrame;
	}
}

void TrackedFrame::eraseContours()
{
	for (size_t flyNumber = 0; flyNumber != flies.size(); ++flyNumber) {
		flies[flyNumber].eraseContours();
	}

	std::vector<cv::Point> emptyCarry0;
	carry0.swap(emptyCarry0);

	std::vector<cv::Point> emptyCarry1;
	carry1.swap(emptyCarry1);
}
