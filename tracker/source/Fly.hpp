#ifndef Fly_hpp
#define Fly_hpp

/*
Stores data about a blob that has been identified during tracking.

The different naming scheme for getters is so that they match with the names given to attributes in FlyAttributes.cpp.
This way the NEW_TRACKING_ATTRIBUTE macro can derive the name of the getter automatically.
*/

#include <stdint.h>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "../../common/source/MyBool.hpp"
#include "../../common/source/Vec.hpp"

class Fly {
public:
	Fly(const cv::Mat& frame, const cv::Mat& foreground, const cv::Mat& bodyMask, const std::vector<std::vector<cv::Point> >& bodyContour, bool bodySplit, size_t bodyContourOffset, size_t bocContourOffset, const std::vector<std::vector<cv::Point> >& wingContour, size_t wingContourOffset);

	float get_bodyAreaTracked() const;
	float get_bodyAreaEccentricityCorrectedTracked() const;
	Vf2 get_bodyCentroidTracked() const;
	//get_bodyBoundingBox();
	float get_bodyMajorAxisLengthTracked() const;
	float get_bodyMinorAxisLengthTracked() const;
	float get_bodyEccentricity() const;
	float get_bodyOrientationTracked() const;
	float get_wingArea() const;
	Vf2 get_wingCentroid() const;
	//get_wingBoundingBox();
	float get_wingMajorAxisLength() const;
	float get_wingMinorAxisLength() const;
	float get_wingEccentricity() const;
	float get_wingOrientation() const;
	float get_wingConvexArea() const;
	Vf2 get_leftTip() const;
	Vf2 get_rightTip() const;
	float get_headingFromWingsTracked() const;
	float get_headingFromColor() const;
	float get_headingFromBody() const;
	std::vector<std::vector<cv::Point> > get_bodyContour() const;
	std::vector<std::vector<cv::Point> > get_wingContour() const;
	MyBool get_bodySplit() const;
	uint32_t get_bodyContourOffset() const;
	uint32_t get_bocContourOffset() const;
	uint32_t get_wingContourOffset() const;
	uint32_t get_bodyContourSize() const;
	uint32_t get_wingContourSize() const;
	Vf2 get_bottomRightWingTip() const;
	Vf2 get_bottomLeftWingTip() const;
	Vf2 get_topLeftWingTip() const;
	Vf2 get_topRightWingTip() const;
	float get_bottomRightBodyArea() const;
	float get_bottomLeftBodyArea() const;
	float get_topLeftBodyArea() const;
	float get_topRightBodyArea() const;
	float get_bottomRightWingArea() const;
	float get_bottomLeftWingArea() const;
	float get_topLeftWingArea() const;
	float get_topRightWingArea() const;
	float get_bottomRightWingAngle() const;
	float get_bottomLeftWingAngle() const;
	float get_topLeftWingAngle() const;
	float get_topRightWingAngle() const;

	void eraseContours();	// to free memory
	
private:
	std::vector<std::vector<cv::Point> > bodyContour;
	std::vector<std::vector<cv::Point> > wingContour;
	bool bodySplit;
	size_t bodyContourOffset;
	size_t bocContourOffset;
	size_t wingContourOffset;
	size_t bodyContourSize;
	size_t wingContourSize;
	cv::RotatedRect bodyEllipseBB;
	cv::RotatedRect wingEllipseBB;
	cv::Rect bodyAreaBB;
	cv::Rect wingAreaBB;
	float headingFromColor;
	float headingFromBody;
	unsigned int bodyPixelCount;
	cv::Point2f bottomRightWingTip;
	cv::Point2f bottomLeftWingTip;
	cv::Point2f topLeftWingTip;
	cv::Point2f topRightWingTip;
	float bottomRightBodyArea;
	float bottomLeftBodyArea;
	float topLeftBodyArea;
	float topRightBodyArea;
	float bottomRightWingArea;
	float bottomLeftWingArea;
	float topLeftWingArea;
	float topRightWingArea;
	float bottomRightWingAngle;
	float bottomLeftWingAngle;
	float topLeftWingAngle;
	float topRightWingAngle;
};

#endif
