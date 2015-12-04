#include "FlyAttributes.hpp"

#include <stdint.h>
#include "../../common/source/Vec.hpp"
#include "../../common/source/MyBool.hpp"
#include "../../common/source/macro.h"
#define NEW_FLY_ATTRIBUTE(group, type, shortname, name, description, unit) attributeMap[STRINGIFY(name)] = new Attribute<type>(shortname, STRINGIFY(name), description, unit); group.push_back(STRINGIFY(name));
#if defined(MATEBOOK_GUI)
	#define NEW_TRACKING_ATTRIBUTE(group, type, shortname, name, description, unit) attributeMap[STRINGIFY(name)] = new Attribute<type>(shortname, STRINGIFY(name), description, unit); group.push_back(STRINGIFY(name));
#else
	#define NEW_TRACKING_ATTRIBUTE(group, type, shortname, name, description, unit) attributeMap[STRINGIFY(name)] = new TrackingAttributeFly<type>(shortname, STRINGIFY(name), description, unit, &Fly::get_##name); group.push_back(STRINGIFY(name));
#endif

FlyAttributes::FlyAttributes() : AttributeCollection()
{
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "bodyContourOffset", bodyContourOffset, "Offset into the contour file to access the body contour.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "", bodyContourSize, "Number of pixels that are part of the body contour.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "wingContourOffset", wingContourOffset, "Offset into the contour file to access the wing contour.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "", wingContourSize, "Number of pixels that are part of the wing contour.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, uint32_t, "", bocContourOffset, "Offset into the contour file to access the boc contour.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, MyBool, "", bodySplit, "Whether the body contour is the result of a split operation.", "");

	// to be interpolated
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bodyAreaTracked, "Fly body area (as reported during tracking).", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bodyAreaEccentricityCorrectedTracked, "Fly body area after correcting for posture (as reported during tracking).", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, Vf2, "", bodyCentroidTracked, "Centroid of the ellipse fit to the fly body (as reported during tracking).", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bodyMajorAxisLengthTracked, "Major axis length of the ellipse fit to the fly body (as reported during tracking).", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bodyMinorAxisLengthTracked, "Minor axis length of the ellipse fit to the fly body (as reported during tracking).", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bodyOrientationTracked, "Global orientation of the ellipse fit to the fly body (as reported during tracking).", "deg");

	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", wingArea, "Fly wing area, which includes the body area.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, Vf2, "", wingCentroid, "Centroid of the ellipse fit to the fly wings.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", wingMajorAxisLength, "Major axis length of the ellipse fit to the fly wings.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", wingMinorAxisLength, "Minor axis length of the ellipse fit to the fly wings.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", wingOrientation, "Global orientation of the ellipse fit to the fly wings.", "deg");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", wingConvexArea, "Area of the convex hull of the wings.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, Vf2, "", bottomRightWingTip, "Potential wing tip in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, Vf2, "", bottomLeftWingTip, "Potential wing tip in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, Vf2, "", topLeftWingTip, "Potential wing tip in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, Vf2, "", topRightWingTip, "Potential wing tip in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bottomRightBodyArea, "Potential body area in one of the wing quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bottomLeftBodyArea, "Potential body area in one of the wing quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", topLeftBodyArea, "Potential body area in one of the wing quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", topRightBodyArea, "Potential body area in one of the wing quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bottomRightWingArea, "Potential wing area in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bottomLeftWingArea, "Potential wing area in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", topLeftWingArea, "Potential wing area in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", topRightWingArea, "Potential wing area in one of the quadrants.", "px");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bottomRightWingAngle, "Potential wing angle in one of the quadrants.", "deg");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", bottomLeftWingAngle, "Potential wing angle in one of the quadrants.", "deg");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", topLeftWingAngle, "Potential wing angle in one of the quadrants.", "deg");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", topRightWingAngle, "Potential wing angle in one of the quadrants.", "deg");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", headingFromWingsTracked, "Heading score based on the position of the wing ellipse relative to the body ellipse (as reported during tracking).", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", headingFromColor, "Heading score based on the color distribution within the fly body.", "");
	NEW_TRACKING_ATTRIBUTE(trackingAttributes, float, "", headingFromBody, "Heading score based on the shape of the fly body.", "");

	// interpolated
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", bodyArea, "Fly body area.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", bodyAreaEccentricityCorrected, "Fly body area after correcting for posture.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "bodyCentroid", bodyCentroid, "Centroid of the ellipse fit to the fly body.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "bodyMajorAxisLength", bodyMajorAxisLength, "Major axis length of the ellipse fit to the fly body.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "bodyMinorAxisLength", bodyMinorAxisLength, "Minor axis length of the ellipse fit to the fly body.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", bodyOrientationFlipped, "Global orientation of the ellipse fit to the fly body, after determining heading.", "deg");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "bori", bodyOrientation, "Global orientation of the ellipse fit to the fly body, after reinterpolating using the final heading.", "deg");

	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", distanceFromArenaCenter, "The distance between the fly body centroid and the center of the arena bounding box.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", headingFromWings, "Heading score based on the position of the wing ellipse relative to the body ellipse (zeroed out for interpolated frames).", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "e", bodyEccentricity, "Eccentricity of the ellipse fit to the fly body.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", wingEccentricity, "Eccentricity of the ellipse fit to the fly wings.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", headingFromBefore, "Heading score based on the persistence of orientation.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", headingFromMotion, "Heading score based on the direction of motion.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", headingFromMaxMotionWings, "The more extreme of headingFromMotion and headingFromWings.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", headingSCombined, "Weighted and combined s values for heading.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", headingTCombined, "Weighted and combined t values for heading.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", filteredHeading, "The decision made for heading.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "", filteredBodyCentroid, "Smoother version of bodyCentroid.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "", moved, "Motion vector since the last frame.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", movedAbs, "The distance moved since the last frame.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", movedDirectionGlobal, "Global motion direction since the last frame.", "deg");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "dir", movedDirectionLocal, "Local motion direction since the last frame.", "deg");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "rot", turned, "The change of orientation since the last frame.", "deg");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", turnedAbs, "The absolute value of the change of orientation since the last frame.", "deg");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "leftWingTip", leftWingTip, "Actual tip of the left wing.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "rightWingTip", rightWingTip, "Actual tip of the right wing.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", leftBodyArea, "Actual area of the body within the left wing quadrant.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", rightBodyArea, "Actual area of the body within the right wing quadrant.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "lws", leftWingArea, "Actual area of the left wing.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "rws", rightWingArea, "Actual area of the right wing.", "px");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "lwa", leftWingAngle, "Actual angle of the left wing.", "deg");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "rwa", rightWingAngle, "Actual angle of the right wing.", "deg");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", oriBelowMaxSpeedSelf, "Speed is below the threshold set for orienting for the active fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", oriBelowMaxSpeedOther, "Speed is below the threshold set for orienting for the passive fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", rayEllipseOriBelowMaxSpeedSelf, "Speed is below the threshold set for orienting for the active fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", rayEllipseOriBelowMaxSpeedOther, "Speed is below the threshold set for orienting for the passive fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", follAboveMinSpeedSelf, "Speed is above the threshold set for following for the active fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", follAboveMinSpeedOther, "Speed is above the threshold set for following for the passive fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", circAboveMinSpeedSelf, "Speed is above the threshold set for circling for the active fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", circBelowMaxSpeedOther, "Speed is below the threshold set for circling for the passive fly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", circMovedSideways, "Fly is moving sideways.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", circAboveMinSidewaysSpeed, "Fly is moving sideways quickly.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtAngleLeft, "Left wing fulfilling the angle criterion.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtAngleRight, "Right wing fulfilling the angle criterion.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtAreaLeft, "Left wing fulfilling the area criterion.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtAreaRight, "Right wing fulfilling the area criterion.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "wel", wingExtLeft, "Left wing extended.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtLeftOccurred, "Whether wingExtLeft has occurred in any frames up to and including this one.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "wer", wingExtRight, "Right wing extended.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtRightOccurred, "Whether wingExtRight has occurred in any frames up to and including this one.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExt, "Any wing extended.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtOccurred, "Whether wingExt has occurred in any frames up to and including this one.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "webi", wingExtBoth, "Both wings extended.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "", wingExtEitherOr, "Exactly one wing extended.", "");
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "cop", copulating, "Fly is copulating.", "");	// same for all flies for now - tricky because it's based on a frameAttribute
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", weightedCourting, "Weighted combination of courtship events.", "");	// tricky because it's based on a fly-attribs (wingExt) and pair-attribs; we should add a weightedCourting pair-attrib in the future
	NEW_FLY_ATTRIBUTE(derivedAttributes, MyBool, "court", courting, "Fly is courting.", "");	// tricky because it's based on a fly-attribs (wingExt) and pair-attribs; we should add a courting pair-attrib in the future

	// attributes resulting from unit conversion
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "s", bodyAreaEccentricityCorrected_u, "Fly body area after correcting for posture.", "mm²");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "cxcy", bodyCentroid_u, "Centroid of the ellipse fit to the fly body.", "mm");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "a", bodyMajorAxisLength_u, "Major axis length of the ellipse fit to the fly body.", "mm");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "b", bodyMinorAxisLength_u, "Minor axis length of the ellipse fit to the fly body.", "mm");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "", distanceFromArenaCenter_u, "The distance between the fly body centroid and the center of the arena bounding box.", "mm");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "", filteredBodyCentroid_u, "Smoother version of bodyCentroid.", "mm");
	NEW_FLY_ATTRIBUTE(derivedAttributes, Vf2, "", moved_u, "Motion vector since the last frame.", "mm");
	NEW_FLY_ATTRIBUTE(derivedAttributes, float, "v", movedAbs_u, "The distance moved since the last frame.", "mm");
}

#if !defined(MATEBOOK_GUI)
void FlyAttributes::appendFly(const Fly& fly)
{
	for (std::vector<std::string>::const_iterator iter = trackingAttributes.begin(); iter != trackingAttributes.end(); ++iter) {
		AbstractAttribute& attribute = get(*iter);
		attribute.tracked(fly);
	}
}

void FlyAttributes::appendEmpty()
{
	for (std::vector<std::string>::const_iterator iter = trackingAttributes.begin(); iter != trackingAttributes.end(); ++iter) {
		AbstractAttribute& attribute = get(*iter);
		attribute.resize(attribute.size() + 1);
	}
}
#endif

void FlyAttributes::swap(FlyAttributes& other, const std::vector<bool>& mask)
{
	assert(attributeMap.size() == other.attributeMap.size());
	AttributeMap::iterator thisIter = attributeMap.begin();
	AttributeMap::iterator otherIter = other.attributeMap.begin();
	while (thisIter != attributeMap.end()) {
		assert(thisIter->first == otherIter->first);
		assert(thisIter->second->size() == otherIter->second->size());
		if (!thisIter->second->empty()) {
			assert(thisIter->second->size() == mask.size());
			thisIter->second->swap(*(otherIter->second), mask);
		}
		++thisIter;
		++otherIter;
	}
}

void FlyAttributes::clearDerivedAttributes()
{
	for (std::vector<std::string>::const_iterator iter = derivedAttributes.begin(); iter != derivedAttributes.end(); ++iter) {
		AbstractAttribute& attribute = get(*iter);
		attribute.clear();
	}
}
