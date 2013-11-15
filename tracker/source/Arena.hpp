#ifndef Arena_hpp
#define Arena_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <boost/shared_ptr.hpp>

#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include "TrackedFrame.hpp"
#include "reconstruct.hpp"
#include "FlyAttributes.hpp"
#include "FrameAttributes.hpp"
#include "PairAttributes.hpp"
#include "OcclusionMap.hpp"

class Arena {
public:
	Arena(const std::string& id, double sourceFrameRate, size_t flyCount, const cv::Rect& boundingBox, float diameter, float borderSize, const cv::Mat& mask, const cv::Mat& background);

	std::string getId() const;
	cv::Rect getBoundingBox() const;
	cv::Mat getMask() const;
	TrackedFrame& frame(size_t i);
	size_t getFrameCount() const;
	size_t getFlyCount() const;
	void track(const cv::Mat& entireFrame, const size_t videoFrameNumber, const size_t videoFrameTotalCount, const size_t trackFrameTotalCount, cv::Mat& visualizedContours, float thresholdOffset, float minFlyBodySizeSquareMillimeter, float maxFlyBodySizeSquareMillimeter, bool gradientCorrection, bool fullyMergeMissegmentations, bool splitBodies, bool splitWings, bool saveContours, bool saveHistograms);
	void normalizeTrackingData();	// converts data to vector of attributes format
	void prepareInterpolation();	// figures out which frames will have to be interpolated
	void buildSequenceMaps();
	void detectMissegmentations(float minFlyBodySizeSquareMillimeter, float maxFlyBodySizeSquareMillimeter);
	void writeSegmentationStatistics(std::ostream& out) const;	// this is done here since the non-interpolated values are available
	void calculateTScores(float tPosLogisticRegressionCoefficient, float tBocLogisticRegressionCoefficient);	//TODO: this can actually be done while tracking
	void solveOcclusions(float sSizeWeight, bool discardMissegmentations);	// rearranges the flyAttributes so that all data in flyAttributes[i] vectors belong to the same fly
	void addAnnotations(const std::string& fileName);
	void writeOcclusionReport(std::ostream& out) const;
	void interpolateAttributes();
	void deriveHeadingIndependentAttributes();
	void solveHeading(float sMotionWeight, float sWingsWeight, float sMaxMotionWingsWeight, float sColorWeight, float tBeforeWeight);
	void interpolateOrientation();
	void selectQuadrants();
	void deriveHeadingDependentAttributes();
	void convertUnits();

	void deriveCopulating(float medianFilterSeconds, float persistence);
	void deriveOrienting(float maxAngle, float minDistance, float maxDistance, float maxSpeedSelf, float maxSpeedOther, float medianFilterWidth, float persistence);
	void deriveRayEllipseOrienting(float growthOther, float maxAngle, float minDistance, float maxDistance, float maxSpeedSelf, float maxSpeedOther, float medianFilterWidth, float persistence);
	void deriveFollowing(float maxChangeOfDistance, float maxAngle, float minDistance, float maxDistance, float minSpeed, float maxMovementDirectionDifference, float medianFilterWidth, float persistence);
	void deriveCircling(float minDistance, float maxDistance, float maxAngle, float minSpeedSelf, float maxSpeedOther, float minAngleDifference, float minSidewaysSpeed, float medianFilterWidth, float persistence);
	void deriveWingExt(float minAngle, float tailQuadrantAreaRatio, float directionTolerance, float minBoc, float angleMedianFilterWidth, float areaMedianFilterWidth, float persistence);
	void deriveCourtship(float circlingWeight, float copulatingWeight, float followingWeight, float orientingWeight, float rayEllipseOrientingWeight, float wingExtWeight);

	void importTrackingData(std::istream& in);
	void exportTrackingData(std::ostream& out) const;
	void exportAttributes(const std::string& outDirPath) const;
	std::vector<FlyAttributes>& getFlyAttributes();
	void writeMean(std::ostream& out) const;
	void writeBehavior(std::ostream& out, float binSize, size_t binCount) const;
	void writeEthograms(const std::string& outDir, const std::string& specification) const;
	void writeMissegmented(std::ostream& out) const;
	void writePositionCorrelation(std::ostream& out) const;

private:
	size_t writeContour(const std::vector<std::vector<cv::Point> >& contour);

	// helper functions for Arena::writeBehavior
	void writeFrameBehavior(std::ostream& out, std::string attributeName, size_t frameEnd, size_t framesPerBin, size_t binCount, const char delimiter = '\t') const;
	void writeFlyBehavior(std::ostream& out, std::string attributeName, size_t frameEnd, size_t framesPerBin, size_t binCount, const char delimiter = '\t') const;
	void writePairBehavior(std::ostream& out, std::string attributeName, size_t frameEnd, size_t framesPerBin, size_t binCount, const char delimiter = '\t') const;

	// helper functions for Arena::writeEthograms
	void paintIntoEthogram(cv::Mat& ethogram, const Attribute<MyBool>& attribute, cv::Vec3b color, size_t paddingLeft, size_t paddingRight, size_t rowBegin, size_t rowEnd) const;

	std::string id;
	double sourceFrameRate;
	float pixelPerMillimeter;
	size_t flyCount;
	cv::Rect boundingBox;
	cv::Mat mask;
	cv::Mat background;
	cv::Mat smoothBackground;
	std::vector<TrackedFrame> frames;

	FrameAttributes frameAttributes;
	std::vector<FlyAttributes> flyAttributes;	// [flyNumber]
	std::vector<std::vector<PairAttributes> > pairAttributes;	// [activeFly][passiveFly]

	boost::shared_ptr<std::ofstream> contourFile;
	size_t contourFileOffset;	// in bytes

	boost::shared_ptr<std::ofstream> smoothHistogramFile;

	OcclusionMap occlusionMap;
};

#endif
