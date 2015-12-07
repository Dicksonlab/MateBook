#include "PairAttributes.hpp"

#include <stdint.h>
#include "../../common/source/MyBool.hpp"
#include "../../common/source/macro.h"
#define NEW_PAIR_ATTRIBUTE(group, type, shortname, name, description, unit) attributeMap[STRINGIFY(name)] = new Attribute<type>(shortname, STRINGIFY(name), description, unit); group.push_back(STRINGIFY(name));

PairAttributes::PairAttributes() : AttributeCollection()
{
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", distanceBodyBody, "Distance between body centroids.", "px");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", distanceHeadBody, "Distance between this fly's head and the other fly's body centroids.", "px");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", distanceHeadTail, "Distance between this fly's head and the other fly's tail.", "px");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", changeInDistanceBodyBody, "Speed at which distanceBodyBody changes.", "px/frame");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", changeInDistanceHeadTail, "Speed at which distanceHeadTail changes.", "px/frame");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", changeInDistanceHeadBody, "Speed at which distanceHeadBody changes.", "px/frame");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "theta", angleToOther, "Angle between the major axis and the line connecting the body centroids.", "deg");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", changeInAngleToOther, "Speed at which angle between the major axis and the line connecting the body centroids changes.", "deg/frame");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "dtheta", changeInAngleToOther_u, "Speed at which angle between the major axis and the line connecting the body centroids changes.", "deg/sec");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "thetaht", angleSubtended, "Angle subtended between the head and the other fly's head and tail.", "deg");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "", changeInAngleSubtended, "Speed at which the angle subtended between the head and the other fly's head and tail changes.", "deg/frame");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "dthetaht", changeInAngleSubtended_u, "Speed at which the angle subtended between the head and the other fly's head and tail changes.", "deg/sec");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, Vf2, "", vectorToOtherLocal, "Vector in this fly's local coordinate frame pointing to the other fly's body centroid.", "px");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", oriAngle, "Orientation angle criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", oriDistance, "Orientation distance criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "ori", orienting, "This fly is orienting itself towards the other fly.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", rayEllipseOriHit, "This fly's viewing ray hits the other fly's body.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", rayEllipseOriAngle, "Orientation angle criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", rayEllipseOriDistance, "Orientation distance criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", rayEllipseOrienting, "This fly is orienting itself towards the other fly.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", follSmallChangeInDistance, "The changeInDistanceHeadBody attribute is below the threshold for following.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", follAngle, "Following angle criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", follDistance, "Following distance criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", follSameMovedDirection, "The flies move in roughly the same direction.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", follBehind, "[TODO: Come up with an intuitive description for this attribute.]", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", following, "This fly is following the other fly.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", followingOccurred, "Whether following has occurred in any frames up to and including this one.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", circAngle, "Circling angle criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "", circDistance, "Circling distance criterion fulfilled.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "circ", circling, "This fly is circling the other fly.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "wet", wingExtTowards, "This fly is extending its wing towards the other fly.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "wea", wingExtAway, "This fly is extending its wing away from the other fly.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "wef", wingExtFront, "This fly is extending a wing while the other fly is in front.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "wei", wingExtIpsi, "This fly is extending the ipsilateral wing.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "wec", wingExtContra, "This fly is extending the contralateral wing.", "");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, MyBool, "web", wingExtBehind, "This fly is extending a wing while the other fly is in back.", "");

	// attributes resulting from unit conversion
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "dcc", distanceBodyBody_u, "Distance between body centroids.", "mm");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "dhc", distanceHeadBody_u, "Distance between this fly's head and the other fly's body centroids.", "mm");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "dht", distanceHeadTail_u, "Distance between this fly's head and the other fly's tail.", "mm");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "ddcc", changeInDistanceBodyBody_u, "Speed at which distanceBodyBody changes.", "mm/s");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "ddht", changeInDistanceHeadTail_u, "Speed at which distanceHeadTail changes.", "mm/s");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, float, "ddhc", changeInDistanceHeadBody_u, "Speed at which distanceHeadBody changes.", "mm/s");
	NEW_PAIR_ATTRIBUTE(derivedAttributes, Vf2, "", vectorToOtherLocal_u, "Vector in this fly's local coordinate frame pointing to the other fly's body centroid.", "mm");
}

void PairAttributes::clearDerivedAttributes()
{
	for (std::vector<std::string>::const_iterator iter = derivedAttributes.begin(); iter != derivedAttributes.end(); ++iter) {
		AbstractAttribute& attribute = get(*iter);
		attribute.clear();
	}
}
