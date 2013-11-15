#include "Fly.hpp"
#include <string>
#include <stdexcept>
#include "euclideanDistance.hpp"
#include "../../common/source/geometry.hpp"
#include "score2prob.hpp"
#include "prob2logodd.hpp"

Fly::Fly(const cv::Mat& frame, const cv::Mat& foreground, const cv::Mat& bodyMask, const std::vector<std::vector<cv::Point> >& bodyContour, bool bodySplit, size_t bodyContourOffset, size_t bocContourOffset, const std::vector<std::vector<cv::Point> >& wingContour, size_t wingContourOffset) :
	bodyContour(bodyContour),
	wingContour(wingContour),
	bodySplit(bodySplit),
	bodyContourOffset(bodyContourOffset),
	bocContourOffset(bocContourOffset),
	wingContourOffset(wingContourOffset),
	bodyContourSize(0),
	wingContourSize(0),
	headingFromColor(),
	headingFromBody(),
	bodyPixelCount(),
	bottomRightBodyArea(),
	bottomLeftBodyArea(),
	topLeftBodyArea(),
	topRightBodyArea(),
	bottomRightWingArea(),
	bottomLeftWingArea(),
	topLeftWingArea(),
	topRightWingArea(),
	bottomRightWingAngle(),
	bottomLeftWingAngle(),
	topLeftWingAngle(),
	topRightWingAngle()
{
	if (foreground.size() != bodyMask.size()) {
		throw std::runtime_error("foreground and bodyMask passed to Fly constructor must have the same size");
	}

	//TODO: currently skipped unless there's a single segment, which we assume to be a closed loop...is there a smart way to do this with multiple segments (e.g. from boc)?
	if (bodyContour.size() == 1) {
		bodyContourSize = bodyContour[0].size();

		if (bodyContour[0].size() < 6) {
			throw std::runtime_error("fitEllipse needs contours with at least 6 points");
		}
		cv::Mat bodyContourAsMat(bodyContour[0]);
		bodyEllipseBB = fitEllipse(bodyContourAsMat);

		// for heading calculations
		cv::Point2f centroid = bodyEllipseBB.center;
		double radAngle = get_bodyOrientationTracked() * CV_PI / 180.;	// in [0,PI)

		{	// heading from color
			float upperCount = 0;
			float lowerCount = 0;
			float upperSumDeltaRG = 0;
			float lowerSumDeltaRG = 0;
			for (int rowIndex = 0; rowIndex != foreground.rows; ++rowIndex) {
				const unsigned char* maskRow = bodyMask.ptr<unsigned char>(rowIndex);
				for (int colIndex = 0; colIndex != foreground.cols; ++colIndex) {
					if (maskRow[colIndex]) {
						++bodyPixelCount;
						cv::Vec3b color = frame.at<cv::Vec3b>(rowIndex, colIndex);
						float red = color[2];
						float green = color[1];
						float blue = color[0];
						float xTranslated = colIndex - centroid.x;
						float yTranslated = rowIndex - centroid.y;
						float xRotated = (float)cos(-radAngle) * xTranslated - (float)sin(-radAngle) * yTranslated;
						//float yRotated = (float)sin(-radAngle) * xTranslated + (float)cos(-radAngle) * yTranslated;
						if (xRotated > 0) {	// lower part (in global coordinate system)
							++lowerCount;
							lowerSumDeltaRG += (red - green);
						} else {	// upper part (in global coordinate system)
							++upperCount;
							upperSumDeltaRG += (red - green);
						}
					}
				}
			}
			if (upperCount == 0 || lowerCount == 0) {
				headingFromColor = 0;
			} else {
				float pooledAverageDeltaRG = (upperSumDeltaRG + lowerSumDeltaRG) / (upperCount + lowerCount);
				float upperAverageDeltaRG = upperSumDeltaRG / upperCount;
				float lowerAverageDeltaRG = lowerSumDeltaRG / lowerCount;
				if (lowerAverageDeltaRG > upperAverageDeltaRG) {	// fly looking down: positive heading
					headingFromColor = (lowerAverageDeltaRG - pooledAverageDeltaRG) / (lowerAverageDeltaRG - upperAverageDeltaRG);
				} else if (upperAverageDeltaRG > lowerAverageDeltaRG) {	// fly looking up: negative heading
					headingFromColor = -(upperAverageDeltaRG - pooledAverageDeltaRG) / (upperAverageDeltaRG - lowerAverageDeltaRG);
				} else {
					headingFromColor = 0;
				}
			}
		}

		{	// heading from body-shape (an s-score assuming the fly body is slimmer in the front than it is in the back)
			float upperCount = 0;
			float lowerCount = 0;
			float upperSum = 0;
			float lowerSum = 0;
			for (size_t segmentIndex = 0; segmentIndex != bodyContour.size(); ++segmentIndex) {
				for (size_t pixelIndex = 0; pixelIndex != bodyContour[segmentIndex].size(); ++pixelIndex) {
					float x = bodyContour[segmentIndex][pixelIndex].x;
					float y = bodyContour[segmentIndex][pixelIndex].y;
					float xTranslated = x - centroid.x;
					float yTranslated = y - centroid.y;
					float xRotated = (float)cos(-radAngle) * xTranslated - (float)sin(-radAngle) * yTranslated;
					float yRotated = (float)sin(-radAngle) * xTranslated + (float)cos(-radAngle) * yTranslated;
					if (xRotated > 0) {	// lower part (in global coordinate system)
						++lowerCount;
						lowerSum += std::abs(yRotated);
					} else {	// upper part (in global coordinate system)
						++upperCount;
						upperSum += std::abs(yRotated);
					}
				}
			}
			if (upperCount == 0 || lowerCount == 0) {
				headingFromBody = 0;
			} else {
				float upperAverage = upperSum / upperCount;
				float lowerAverage = lowerSum / lowerCount;
				headingFromBody = (upperAverage - lowerAverage) / (upperAverage + lowerAverage);	// positive (meaning fly heading down) when the body is wider on the upper part
			}
		}

		// calculate body quadrant areas
		const std::vector<cv::Point>& segment = bodyContour[0];
		for (size_t pointNumber = 0; pointNumber != segment.size(); ++pointNumber) {
			cv::Point currentPoint = segment[pointNumber];
			cv::Point nextPoint = (pointNumber + 1 != segment.size() ? segment[pointNumber + 1] : segment[0]);

			float currentDeltaX = currentPoint.x - centroid.x;
			float currentDeltaY = currentPoint.y - centroid.y;
			float nextDeltaX = nextPoint.x - centroid.x;
			float nextDeltaY = nextPoint.y - centroid.y;
			float deltaArea = currentDeltaX * nextDeltaY - nextDeltaX * currentDeltaY;	// parallelogram area based on the cross product

			// check which quadrants currentPoint is in
			// quadrants are defined as the sectors from 10° to 100° on each side of the major axis
			// so both left quadrants and both right quadrants are overlapping, but only one of them is chosen later - when heading is known
			float currentPointGlobalAngle = atan2(currentPoint.y - centroid.y, currentPoint.x - centroid.x) * 180 / CV_PI;
			float angleDiff = angleDifference(get_bodyOrientationTracked(), currentPointGlobalAngle);
			if (angleDiff < -10 && angleDiff > -100) {	// bottom right
				bottomRightBodyArea += deltaArea;
			}
			if (angleDiff > 10 && angleDiff < 100) {	// bottom left
				bottomLeftBodyArea += deltaArea;
			}
			if (angleDiff > 80 && angleDiff < 170) {	// top left
				topLeftBodyArea += deltaArea;
			}
			if (angleDiff < -80 && angleDiff > -170) {	// top right
				topRightBodyArea += deltaArea;
			}
		}
		// take the absolute value in case we traced the wrong way around the area
		// divide by 2 to go from parallelogram area to triangle area
		bottomRightBodyArea = std::abs(bottomRightBodyArea) / 2;
		bottomLeftBodyArea = std::abs(bottomLeftBodyArea) / 2;
		topLeftBodyArea = std::abs(topLeftBodyArea) / 2;
		topRightBodyArea = std::abs(topRightBodyArea) / 2;

		// calculate wing attributes
		//TODO: currently skipped unless there's a single segment, which we assume to be a closed loop...is there a smart way to do this with multiple segments (e.g. from boc)?
		if (wingContour.size() == 1) {
			wingContourSize = wingContour[0].size();

			if (wingContour[0].size() < 6) {
				throw std::runtime_error("fitEllipse needs contours with at least 6 points");
			}
			cv::Mat wingContourAsMat(wingContour[0]);
			wingEllipseBB = fitEllipse(wingContourAsMat);

			const std::vector<cv::Point>& segment = wingContour[0];

			// we don't know the heading yet, so we determine them for 4 quadrants
			// the prefixes ("bottomRight",...) refer to the global coordinate system
			float bottomRightDistance = 0;
			float bottomLeftDistance = 0;
			float topLeftDistance = 0;
			float topRightDistance = 0;

			for (size_t pointNumber = 0; pointNumber != segment.size(); ++pointNumber) {
				cv::Point currentPoint = segment[pointNumber];
				cv::Point nextPoint = (pointNumber + 1 != segment.size() ? segment[pointNumber + 1] : segment[0]);

				float currentDeltaX = currentPoint.x - centroid.x;
				float currentDeltaY = currentPoint.y - centroid.y;
				float nextDeltaX = nextPoint.x - centroid.x;
				float nextDeltaY = nextPoint.y - centroid.y;
				float deltaArea = currentDeltaX * nextDeltaY - nextDeltaX * currentDeltaY;	// parallelogram area based on the cross product

				float currentDistance = euclideanDistance<float>(centroid, currentPoint);

				// check which quadrants currentPoint is in
				// quadrants are defined as the sectors from 10° to 100° on each side of the major axis
				// so both left quadrants and both right quadrants are overlapping, but only one of them is chosen later - when heading is known
				float currentPointGlobalAngle = atan2(currentPoint.y - centroid.y, currentPoint.x - centroid.x) * 180 / CV_PI;
				float angleDiff = angleDifference(get_bodyOrientationTracked(), currentPointGlobalAngle);
				if (angleDiff < -10 && angleDiff > -100) {	// bottom right
					if (currentDistance > bottomRightDistance) {
						bottomRightWingTip = currentPoint;
						bottomRightDistance = currentDistance;
						bottomRightWingAngle = -angleDiff;
					}
					bottomRightWingArea += deltaArea;
				}
				if (angleDiff > 10 && angleDiff < 100) {	// bottom left
					if (currentDistance > bottomLeftDistance) {
						bottomLeftWingTip = currentPoint;
						bottomLeftDistance = currentDistance;
						bottomLeftWingAngle = angleDiff;
					}
					bottomLeftWingArea += deltaArea;
				}
				if (angleDiff > 80 && angleDiff < 170) {	// top left
					if (currentDistance > topLeftDistance) {
						topLeftWingTip = currentPoint;
						topLeftDistance = currentDistance;
						topLeftWingAngle = 180 - angleDiff;
					}
					topLeftWingArea += deltaArea;
				}
				if (angleDiff < -80 && angleDiff > -170) {	// top right
					if (currentDistance > topRightDistance) {
						topRightWingTip = currentPoint;
						topRightDistance = currentDistance;
						topRightWingAngle = 180 + angleDiff;
					}
					topRightWingArea += deltaArea;
				}
			}
			// take the absolute value in case we traced the wrong way around the area
			// divide by 2 to go from parallelogram area to triangle area
			bottomRightWingArea = std::abs(bottomRightWingArea) / 2;
			bottomLeftWingArea = std::abs(bottomLeftWingArea) / 2;
			topLeftWingArea = std::abs(topLeftWingArea) / 2;
			topRightWingArea = std::abs(topRightWingArea) / 2;
		}
	}
}

float Fly::get_bodyAreaTracked() const
{
	return bodyPixelCount;
}

float Fly::get_wingArea() const
{
	return 0;	//TODO
}

float Fly::get_wingConvexArea() const
{
	return 0;	//TODO
}

float Fly::get_bodyAreaEccentricityCorrectedTracked() const
{
	//TODO: justification? see normalizeTrack2.m#297 and thesis#106
	float eccentricity = get_bodyEccentricity();
	float eccentricityCorrected = eccentricity / (1 + exp(-5 * (eccentricity - 0.5)));
	return get_bodyAreaTracked() * sqrt(1 - eccentricityCorrected * eccentricityCorrected);
}

Vf2 Fly::get_bodyCentroidTracked() const
{
	Vf2 ret;
	ret.x() = bodyEllipseBB.center.x;
	ret.y() = bodyEllipseBB.center.y;
	return ret;
}

Vf2 Fly::get_wingCentroid() const
{
	Vf2 ret;
	ret.x() = wingEllipseBB.center.x;
	ret.y() = wingEllipseBB.center.y;
	return ret;
}

float Fly::get_bodyMajorAxisLengthTracked() const
{
	return std::max(bodyEllipseBB.size.width, bodyEllipseBB.size.height);
}

float Fly::get_bodyMinorAxisLengthTracked() const
{
	return std::min(bodyEllipseBB.size.width, bodyEllipseBB.size.height);
}

float Fly::get_wingMajorAxisLength() const
{
	return std::max(wingEllipseBB.size.width, wingEllipseBB.size.height);
}

float Fly::get_wingMinorAxisLength() const
{
	return std::min(wingEllipseBB.size.width, wingEllipseBB.size.height);
}

float Fly::get_bodyEccentricity() const
{
	return eccentricity(get_bodyMajorAxisLengthTracked(), get_bodyMinorAxisLengthTracked());
}

float Fly::get_wingEccentricity() const
{
	return eccentricity(get_wingMajorAxisLength(), get_wingMinorAxisLength());
}

float Fly::get_bodyOrientationTracked() const
{
	float ret = bodyEllipseBB.angle - 90;
	if (ret < 0) {
		ret += 180;
	}
	if (ret >= 180) {
		ret -= 180;
	}
	// ret should now be in [0,180)
	return ret;
}

float Fly::get_wingOrientation() const
{
	float ret = wingEllipseBB.angle - 90;
	if (ret < 0) {
		ret += 180;
	}
	if (ret >= 180) {
		ret -= 180;
	}
	// ret should now be in [0,180)
	return ret;
}

Vf2 Fly::get_leftTip() const
{
	double radAngle = get_bodyOrientationTracked() * CV_PI / 180.;
	float c = (float)cos(radAngle) * 0.5f;
	float s = (float)sin(radAngle) * 0.5f;

	cv::Point2f leftTip = bodyEllipseBB.center;
	leftTip.x -= c * get_bodyMajorAxisLengthTracked();
	leftTip.y -= s * get_bodyMajorAxisLengthTracked();

	Vf2 ret;
	ret.x() = leftTip.x;
	ret.y() = leftTip.y;
	return ret;
}

Vf2 Fly::get_rightTip() const
{
	double radAngle = get_bodyOrientationTracked() * CV_PI / 180.;
	float c = (float)cos(radAngle) * 0.5f;
	float s = (float)sin(radAngle) * 0.5f;

	cv::Point2f rightTip = bodyEllipseBB.center;
	rightTip.x += c * get_bodyMajorAxisLengthTracked();
	rightTip.y += s * get_bodyMajorAxisLengthTracked();

	Vf2 ret;
	ret.x() = rightTip.x;
	ret.y() = rightTip.y;
	return ret;
}

// returns negative value for left-facing flies; positive ones for right-facing flies
float Fly::get_headingFromWingsTracked() const
{
	float distLW = (get_leftTip() - get_wingCentroid()).norm();
	float distRW = (get_rightTip() - get_wingCentroid()).norm();

	float score = (distRW - distLW) / (distRW + distLW);
	return prob2logodd(score2prob(score));
}

float Fly::get_headingFromColor() const
{
	return prob2logodd(score2prob(headingFromColor));
}

float Fly::get_headingFromBody() const
{
	return prob2logodd(score2prob(headingFromBody));
}

std::vector<std::vector<cv::Point> > Fly::get_bodyContour() const
{
	return bodyContour;
}

std::vector<std::vector<cv::Point> > Fly::get_wingContour() const
{
	return wingContour;
}

MyBool Fly::get_bodySplit() const
{
	return bodySplit;
}

uint32_t Fly::get_bodyContourOffset() const
{
	return bodyContourOffset;
}

uint32_t Fly::get_bocContourOffset() const
{
	return bocContourOffset;
}

uint32_t Fly::get_wingContourOffset() const
{
	return wingContourOffset;
}

uint32_t Fly::get_bodyContourSize() const
{
	return bodyContourSize;
}

uint32_t Fly::get_wingContourSize() const
{
	return wingContourSize;
}

void Fly::eraseContours()
{
	std::vector<std::vector<cv::Point> > emptyBodyContour;
	bodyContour.swap(emptyBodyContour);
	
	std::vector<std::vector<cv::Point> > emptyWingContour;
	wingContour.swap(emptyWingContour);
}

Vf2 Fly::get_bottomRightWingTip() const
{
	Vf2 ret;
	ret.x() = bottomRightWingTip.x;
	ret.y() = bottomRightWingTip.y;
	return ret;
}

Vf2 Fly::get_bottomLeftWingTip() const
{
	Vf2 ret;
	ret.x() = bottomLeftWingTip.x;
	ret.y() = bottomLeftWingTip.y;
	return ret;
}

Vf2 Fly::get_topLeftWingTip() const
{
	Vf2 ret;
	ret.x() = topLeftWingTip.x;
	ret.y() = topLeftWingTip.y;
	return ret;
}

Vf2 Fly::get_topRightWingTip() const
{
	Vf2 ret;
	ret.x() = topRightWingTip.x;
	ret.y() = topRightWingTip.y;
	return ret;
}

float Fly::get_bottomRightBodyArea() const
{
	return bottomRightBodyArea;
}

float Fly::get_bottomLeftBodyArea() const
{
	return bottomLeftBodyArea;
}

float Fly::get_topLeftBodyArea() const
{
	return topLeftBodyArea;
}

float Fly::get_topRightBodyArea() const
{
	return topRightBodyArea;
}

float Fly::get_bottomRightWingArea() const
{
	return bottomRightWingArea;
}

float Fly::get_bottomLeftWingArea() const
{
	return bottomLeftWingArea;
}

float Fly::get_topLeftWingArea() const
{
	return topLeftWingArea;
}

float Fly::get_topRightWingArea() const
{
	return topRightWingArea;
}

float Fly::get_bottomRightWingAngle() const
{
	return bottomRightWingAngle;
}

float Fly::get_bottomLeftWingAngle() const
{
	return bottomLeftWingAngle;
}

float Fly::get_topLeftWingAngle() const
{
	return topLeftWingAngle;
}

float Fly::get_topRightWingAngle() const
{
	return topRightWingAngle;
}
