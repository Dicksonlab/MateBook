#include "FrameAttributes.hpp"

#include <algorithm>
#include <stdint.h>
#include "../../common/source/Vec.hpp"
#include "../../common/source/MyBool.hpp"
#include "../../common/source/macro.h"
#define NEW_FRAME_ATTRIBUTE(group, type, shortname, name, description, unit) attributeMap[STRINGIFY(name)] = new Attribute<type>(shortname, STRINGIFY(name), description, unit); group.push_back(STRINGIFY(name));
#if defined(MATEBOOK_GUI)
	#define NEW_TRACKING_ATTRIBUTE(group, type, shortname, name, description, unit) attributeMap[STRINGIFY(name)] = new Attribute<type>(shortname, STRINGIFY(name), description, unit); group.push_back(STRINGIFY(name));
#else
	#define NEW_TRACKING_ATTRIBUTE(group, type, shortname, name, description, unit) attributeMap[STRINGIFY(name)] = new TrackingAttributeFrame<type>(shortname, STRINGIFY(name), description, unit, &TrackedFrame::get_##name); group.push_back(STRINGIFY(name));
#endif

FrameAttributes::FrameAttributes() : AttributeCollection()
{
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "bodyContourOffset", bodyContourOffset, "Offset into the contour file to access the body contours for this (occluded) frame.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "wingContourOffset", wingContourOffset, "Offset into the contour file to access the wing contours for this (occluded) frame.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "bodyThreshold", bodyThreshold, "The threshold picked for body segmentation.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "wingThreshold", wingThreshold, "The threshold picked for wing segmentation.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, MyBool, "", isOcclusionTouched, "Whether this frame is occluded due to flies touching.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, MyBool, "mseg", isMissegmented, "Whether this frame has been missegmented.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "t", videoTime, "Time passed since the beginning of the video.", "s");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", trackedTime, "Time passed since the beginning of the tracked sequence.", "s");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "f", videoFrame, "Index of the video frame, relative to the beginning of the video.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "", trackedFrame, "Index of the video frame, relative to the beginning of the tracked sequence.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", videoFrameRelative, "0.0 is the beginning of the video, while 1.0 is the end.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", trackedFrameRelative, "0.0 is the beginning of the tracked sequence, while 1.0 is the end.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, Vf2, "", averageBodyCentroid, "Average of all body centroids.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", tBocScore, "Contour-based score in [-1,1] for occlusion resolution.", "");

	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "occ", isOcclusion, "Whether this frame is occluded.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "occlusionInspected", occlusionInspected, "Whether this frame is part of an occlusion that has been been switched by the user at some point.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", isMissegmentedUnmerged, "Missegmentations that have not been merged into occlusions.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", interpolated, "Body attributes are interpolated.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", interpolatedAbsolutely, "Body attributes are interpolated absolutely.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", interpolatedRelatively, "Body attributes are interpolated relative to the averageBodyCentroid.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", isMissegmentedSS, "Both flies are too small.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", isMissegmentedS, "One fly is too small, the other one okay.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", isMissegmentedSL, "One fly is too small, the other one too large.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", isMissegmentedL, "One fly is too large, the other one okay.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", isMissegmentedLL, "Both flies are too large.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", sSizeProb, "Size-based probability for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", sSize, "Size-based logodd for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "sCombined", sCombined, "Weighted and combined s values for non-occluded sequences.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tBocProb, "Contour-based probability for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tBoc, "Contour-based logodd for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tPosScore, "Position-based score in [-1,1] for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tPosProb, "Position-based probability for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tPos, "Position-based logodd for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tMovScore, "Motion-based score in [-1,1] for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tMovProb, "Motion-based probability for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "", tMov, "Motion-based logodd for occlusion resolution.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, float, "tCombined", tCombined, "Weighted and combined t values for occluded sequences.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, uint32_t, "", idPermutation, "In a sorted list of permutations, this is the index of the permutation that was applied to the flies to assign the correct identities.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", courtship, "Whether any flies are courting in this frame.", "");
	NEW_FRAME_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtCallableDuringOcclusion, "Whether wingExt can be called during occlusions.", "");

	//TODO: instead of adding all this knowledge to FrameAttributes, how about just deriving ScoreAttribute and ProbAttribute from Attribute and make a virtual swap?
	occlusionScoreAttributes.push_back("tBocScore");
	occlusionScoreAttributes.push_back("tPosScore");
	occlusionScoreAttributes.push_back("tMovScore");
	occlusionScoreAttributes.push_back("sSize");
	occlusionScoreAttributes.push_back("sCombined");
	occlusionScoreAttributes.push_back("tBoc");
	occlusionScoreAttributes.push_back("tPos");
	occlusionScoreAttributes.push_back("tMov");
	occlusionScoreAttributes.push_back("tCombined");

	occlusionSScoreAttributes.push_back("sSize");
	occlusionSScoreAttributes.push_back("sCombined");

	occlusionTScoreAttributes.push_back("tBocScore");
	occlusionTScoreAttributes.push_back("tPosScore");
	occlusionTScoreAttributes.push_back("tMovScore");
	occlusionTScoreAttributes.push_back("tBoc");
	occlusionTScoreAttributes.push_back("tPos");
	occlusionTScoreAttributes.push_back("tMov");
	occlusionTScoreAttributes.push_back("tCombined");

	occlusionProbAttributes.push_back("sSizeProb");
	occlusionProbAttributes.push_back("tBocProb");
	occlusionProbAttributes.push_back("tPosProb");
	occlusionProbAttributes.push_back("tMovProb");

	occlusionSProbAttributes.push_back("sSizeProb");

	occlusionTProbAttributes.push_back("tBocProb");
	occlusionTProbAttributes.push_back("tPosProb");
	occlusionTProbAttributes.push_back("tMovProb");
}

#if !defined(MATEBOOK_GUI)
void FrameAttributes::appendFrame(const TrackedFrame& frame)
{
	for (std::vector<std::string>::const_iterator iter = trackingAttributes.begin(); iter != trackingAttributes.end(); ++iter) {
		AbstractAttribute& attribute = get(*iter);
		attribute.tracked(frame);
	}
}
#endif

void FrameAttributes::swap(const std::vector<bool>& mask)
{
	for (std::vector<std::string>::const_iterator iter = occlusionScoreAttributes.begin(); iter != occlusionScoreAttributes.end(); ++iter) {
		Attribute<float>& occlusionScoreAttribute = getFilled<float>(*iter);
		for (size_t frameNumber = 0; frameNumber != mask.size(); ++frameNumber) {
			if (mask[frameNumber]) {
				occlusionScoreAttribute[frameNumber] = -occlusionScoreAttribute[frameNumber];
			}
		}
	}

	for (std::vector<std::string>::const_iterator iter = occlusionProbAttributes.begin(); iter != occlusionProbAttributes.end(); ++iter) {
		Attribute<float>& occlusionProbAttribute = getFilled<float>(*iter);
		for (size_t frameNumber = 0; frameNumber != mask.size(); ++frameNumber) {
			if (mask[frameNumber]) {
				occlusionProbAttribute[frameNumber] = 1 - occlusionProbAttribute[frameNumber];
			}
		}
	}

	// we use idPermutation to keep track of swaps; TODO: make this work for more than 2 flies
	Attribute<uint32_t>& idPermutation = getFilled<uint32_t>("idPermutation");
	for (size_t frameNumber = 0; frameNumber != mask.size(); ++frameNumber) {
		if (mask[frameNumber]) {
			idPermutation[frameNumber] = 1 - idPermutation[frameNumber];
		}
	}
}

void FrameAttributes::clearDerivedAttributes()
{
	for (std::vector<std::string>::const_iterator iter = derivedAttributes.begin(); iter != derivedAttributes.end(); ++iter) {
		AbstractAttribute& attribute = get(*iter);
		attribute.clear();
	}
}

bool FrameAttributes::isOcclusionSScore(std::string name) const
{
	return std::find(occlusionSScoreAttributes.begin(), occlusionSScoreAttributes.end(), name) != occlusionSScoreAttributes.end();
}

bool FrameAttributes::isOcclusionTScore(std::string name) const
{
	return std::find(occlusionTScoreAttributes.begin(), occlusionTScoreAttributes.end(), name) != occlusionTScoreAttributes.end();
}

bool FrameAttributes::isOcclusionSProb(std::string name) const
{
	return std::find(occlusionSProbAttributes.begin(), occlusionSProbAttributes.end(), name) != occlusionSProbAttributes.end();
}

bool FrameAttributes::isOcclusionTProb(std::string name) const
{
	return std::find(occlusionTProbAttributes.begin(), occlusionTProbAttributes.end(), name) != occlusionTProbAttributes.end();
}

std::vector<std::string> FrameAttributes::getOcclusionSScoreNames() const
{
	return occlusionSScoreAttributes;
}

std::vector<std::string> FrameAttributes::getOcclusionTScoreNames() const
{
	return occlusionTScoreAttributes;
}

std::vector<std::string> FrameAttributes::getOcclusionSProbNames() const
{
	return occlusionSProbAttributes;
}

std::vector<std::string> FrameAttributes::getOcclusionTProbNames() const
{
	return occlusionTProbAttributes;
}

