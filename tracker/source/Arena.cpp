#include "Arena.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <numeric>
#include <limits>
#include <cstdlib>
#include <stdint.h>
#include "global.hpp"
#include "../../common/source/serialization.hpp"
#include "getBodyThreshold.hpp"
#include "inpaint.hpp"
#include "../../common/source/ordfilt.hpp"
#include "../../common/source/gaussian.hpp"
#include "../../common/source/convolve.hpp"
#include "hofacker.hpp"
#include "drawFly.hpp"
#include "euclideanDistance.hpp"
#include "hungarian.hpp"
#include "../../common/source/interpolate.hpp"
#include "score2prob.hpp"
#include "prob2logodd.hpp"
#include "SequenceMap.hpp"
#include "../../common/source/geometry.hpp"
#include "../../common/source/stringUtilities.hpp"
#include "../../common/source/arrayOperations.hpp"
#include "areaFromContour.hpp"
#include "../../common/source/fileUtilities.hpp"

// for removing small contours that would trip up fitEllipse
bool fewerThan6(const std::vector<cv::Point>& contour)
{
	return contour.size() < 6;
}

// for sorting contours
template<class T>
bool bigger(const T& left, const T& right)
{
	return left.size() > right.size();
}

Arena::Arena(const std::string& id, double sourceFrameRate, size_t flyCount, const cv::Rect& boundingBox, float diameter, float borderSize, const cv::Mat& mask, const cv::Mat& background) :
	id(id),
	sourceFrameRate(sourceFrameRate),
	pixelPerMillimeter(boundingBox.width / (diameter + borderSize)),	// findArenas enlarges the bounding box, so we take the borderSize into account here as well
	flyCount(flyCount),
	boundingBox(boundingBox),
	mask(mask),
	background(background),
	contourFileOffset(0)
{
	if (boundingBox.size() != mask.size()) {
		throw std::runtime_error("boundingBox and mask passed to Arena constructor must have the same size");
	}

	makeDirectory(global::outDir + "/" + getId());	//TODO: we need a ternary return type for makeDirectory: CREATED / EXISTS / DOESNT_EXIST_BUT_COULDNT_CREATE

	cv::Mat inpaintMask;
	int strelSize = static_cast<int>(2 * borderSize * pixelPerMillimeter);	//TODO: remove 2* ?
	erode(mask, inpaintMask, cv::Mat(strelSize, strelSize, CV_8UC1, 255));
	smoothBackground = ::inpaint(background, inpaintMask);

	imwrite(global::outDir + "/" + getId() + "/smoothBackground.png", smoothBackground);
}

std::string Arena::getId() const
{
	return id;
}

cv::Rect Arena::getBoundingBox() const
{
	return boundingBox;
}

cv::Mat Arena::getMask() const
{
	return mask;
}

TrackedFrame& Arena::frame(size_t i)
{
	return frames[i];
}

size_t Arena::getFrameCount() const
{
	if (frames.empty()) {	// after normalizeTrackingData has been called
		return frameAttributes.get<uint32_t>("trackedFrame").getData().size();
	}
	// before normalizeTrackingData() has finished
	return frames.size();
}

size_t Arena::getFlyCount() const
{
	return flyCount;
}

struct MergeableBodyContour {
	std::vector<cv::Point> contour;
	cv::Point centroid;
	int wingIndex;
	bool mergeable;	// false when there are no other bodies in the same wing
	bool split;	// whether the area is resulting from a split
};

struct SplittableWingContour {
	cv::Mat mask;
	std::set<size_t> bodyIndexes;	// into the mergeableBodyContours array
	bool split;	// whether the area is resulting from a split
};

// remove vertically moving wave as found in some of our older movies by subtracting from each row its median
cv::Mat removeVerticalWave(const cv::Mat& image)
{
	cv::Mat ret = image.clone();
	for (int row = 0; row != ret.rows; ++row) {
		unsigned char* retRowPointer = ret.ptr<uchar>(row);
		unsigned char* medianPointer = retRowPointer + ret.cols / 2;
		std::nth_element(retRowPointer, medianPointer, retRowPointer + ret.cols);
		unsigned char median = *medianPointer;
		const unsigned char* imageRowPointer = image.ptr<uchar>(row);
		for (int col = 0; col != ret.cols; ++col) {
			if (imageRowPointer[col] >= median) {
				retRowPointer[col] = imageRowPointer[col] - median;
			} else {
				// saturate
				retRowPointer[col] = 0;
			}
		}
	}
	return ret;
}

// stretch the histogram of an image so that values between newMin and newMax are mapped to the full range of the data type
cv::Mat stretch(const cv::Mat& image, const unsigned char newMin, const unsigned char newMax)
{
	cv::Mat ret = image.clone();
	float factor = 255.0f / (newMax - newMin);
	for (size_t i = 0; i != image.rows * image.cols; ++i) {
		float newValue = (image.data[i] - newMin) * factor;
		if (newValue < 0.0f) {
			newValue = 0.0f;
		}
		if (newValue >= 255.0f) {
			newValue = 255.0f;
		}
		ret.data[i] = static_cast<unsigned char>(newValue);
	}
	return ret;
}

// grow the bw-image given in seed to edges found in image (but only allow filling in mask) and return the grown bw-image
cv::Mat gradientCorrect(const cv::Mat& image, const cv::Mat& seed, const cv::Mat& mask)
{
	cv::Mat ret = seed.clone();
	cv::Mat grayFrame;
	cvtColor(image, grayFrame, CV_BGR2GRAY);
	cv::Mat horizontalKernel;
	cv::Mat verticalKernel;
	cv::Mat horizontalEdges;
	cv::Mat verticalEdges;
	cv::getDerivKernels(horizontalKernel, verticalKernel, 1, 1, 1);
	filter2D(grayFrame, horizontalEdges, CV_32F, horizontalKernel);
	filter2D(grayFrame, verticalEdges, CV_32F, verticalKernel);
	cv::Mat gradientSquared = horizontalEdges.mul(horizontalEdges) + verticalEdges.mul(verticalEdges);
	assert(gradientSquared.size() == image.size());

	// use the median of the gradientSquared values within bodies to threshold the gradientSquared image
	std::vector<float> gradientSquaredInBodies;
	size_t bodyPixelCount = 0;	// needed later to check whether the flood-fill leaks or not
	assert(seed.isContinuous() && gradientSquared.isContinuous() && seed.size() == gradientSquared.size());
	for (int row = 0; row != seed.rows; ++row) {
		for (int col = 0; col != seed.cols; ++col) {
			if (seed.at<bool>(row, col)) {
				gradientSquaredInBodies.push_back(gradientSquared.at<float>(row, col));
				++bodyPixelCount;
			}
		}
	}
	if (!gradientSquaredInBodies.empty()) {	//TODO: when does this happen?
		std::vector<float>::iterator medianIter = gradientSquaredInBodies.begin() + gradientSquaredInBodies.size() / 2;
		std::nth_element(gradientSquaredInBodies.begin(), medianIter, gradientSquaredInBodies.end());
		cv::Mat bwGradientSquaredFloat;
		threshold(gradientSquared, bwGradientSquaredFloat, *medianIter, 255, cv::THRESH_BINARY);
		cv::Mat bwGradientSquared;
		bwGradientSquaredFloat.convertTo(bwGradientSquared, CV_8UC1);

		// flood-fill
		for (int strelSize = 1; strelSize <= 5; ++strelSize) {
			cv::Mat fillMarker = (seed & ~bwGradientSquared & mask) > 0;
			morphologyEx(fillMarker, fillMarker, cv::MORPH_OPEN, cv::Mat(strelSize, strelSize, CV_8UC1, 255));

			cv::Mat fillMask = (~bwGradientSquared & mask) > 0;
			morphologyEx(fillMask, fillMask, cv::MORPH_OPEN, cv::Mat(strelSize, strelSize, CV_8UC1, 255));

			cv::Mat reconstructed = 255 * reconstruct(fillMarker, fillMask, 4);
			reconstructed = reconstructed | seed;
			morphologyEx(reconstructed, reconstructed, cv::MORPH_CLOSE, cv::Mat(3, 3, CV_8UC1, 255));

			size_t newBodyPixelCount = 0;	// needed later to check whether the flood-fill leaks or not
			assert(reconstructed.isContinuous());	// not really necessary, since we're using .at below
			for (int row = 0; row != reconstructed.rows; ++row) {
				for (int col = 0; col != reconstructed.cols; ++col) {
					if (reconstructed.at<bool>(row, col)) {
						++newBodyPixelCount;
					}
				}
			}

			if (newBodyPixelCount / bodyPixelCount < 10) {
				// no leak
				ret = reconstructed;
				break;
			}
		}
	}
	return ret;
}

void Arena::track(const cv::Mat& entireFrame, const size_t videoFrameNumber, const size_t videoFrameTotalCount, const size_t trackFrameTotalCount, cv::Mat& visualizedContours, float thresholdOffset, float minFlyBodySizeSquareMillimeter, float maxFlyBodySizeSquareMillimeter, bool gradientCorrection, bool fullyMergeMissegmentations, bool splitBodies, bool splitWings, bool saveContours, bool saveHistograms)
{
	if (frames.empty() && saveContours) {	// this is the first frame we have tracked, so we have to open the contourFile
		std::string contourFileName(global::outDir + "/" + getId() + "/contour.bin");
		contourFile = boost::shared_ptr<std::ofstream>(new std::ofstream(contourFileName.c_str(), std::ios::out | std::ios::binary));
		// we are adding an empty contour at the beginning of the file by writing a 0 (meaning "zero segments")
		// flies and frames can point to it using an offset of 0 when the contour is missing
		const size_t noSegments = 0;
		contourFile->write((char*)&noSegments, sizeof(noSegments)); contourFileOffset += sizeof(noSegments);
	}

	bool saveDebugImages = false;
	bool missegmented = false;

	size_t bodyContourOffset = 0;
	size_t wingContourOffset = 0;

	// boc-related ... created here in case we skip boc with a goto
	// the bocContours vector is currently used for visualization and body-splitting only
	// the boc occlusion resolution method still uses the carryX vectors
	std::vector<std::vector<std::vector<cv::Point> > > bocContours(getFlyCount());	// [fly][segment][vertex]
	std::vector<cv::Point> carry0;
	std::vector<cv::Point> carry1;
	float bocScoreCalculated = 0.0f;	// calculated at the end of an occlusion

	cv::Mat arenaContours(visualizedContours, getBoundingBox());

	cv::Mat frame(entireFrame, getBoundingBox());

	cv::Mat smoothForeground;
	cvtColor(smoothBackground - frame, smoothForeground, CV_BGR2GRAY);
	smoothForeground = smoothForeground & mask;

//	for (int row = 0; row != arenaContours.rows; ++row) {
//		for (int col = 0; col != arenaContours.cols; ++col) {
//			arenaContours.ptr<uchar>(row)[3*col] = smoothForeground.ptr<uchar>(row)[col];
//			arenaContours.ptr<uchar>(row)[3*col+1] = smoothForeground.ptr<uchar>(row)[col];
//			arenaContours.ptr<uchar>(row)[3*col+2] = smoothForeground.ptr<uchar>(row)[col];
//		}
//	}

	if (saveHistograms) {
		// write the histograms from the current frame
		if (frames.empty()) {	// this is the first frame we have tracked, so we have to open the histogramFile first
			std::string fileName(global::outDir + "/" + getId() + "/smoothHistogram.bin256f");
			smoothHistogramFile = boost::shared_ptr<std::ofstream>(new std::ofstream(fileName.c_str(), std::ios::out | std::ios::binary));
		}
	}

	unsigned char bodyThreshold = getBodyThreshold(smoothForeground, smoothHistogramFile);

	// adjust the threshold with an offset (TODO: moonwalker uses bodyThreshold += 15 hack)
	int threshOffset = static_cast<int>(thresholdOffset);
	if (-threshOffset > bodyThreshold) {
		bodyThreshold = 0;
	} else if (threshOffset > std::numeric_limits<unsigned char>::max() - bodyThreshold) {
		bodyThreshold = std::numeric_limits<unsigned char>::max();
	} else {
		bodyThreshold += threshOffset;
	}

	cv::Mat bwBodies;
	threshold(smoothForeground, bwBodies, bodyThreshold, 255, cv::THRESH_BINARY);
	morphologyEx(bwBodies, bwBodies, cv::MORPH_OPEN, cv::Mat(4, 4, CV_8UC1, 255));

	if (gradientCorrection) {
		const unsigned char minDifferenceToFill = 40;
		bwBodies = gradientCorrect(frame, bwBodies, mask & (smoothForeground >= minDifferenceToFill));
	}

	// determine the number of body contour pixels
	std::vector<std::vector<cv::Point> > allBodyContours;
	cv::Mat bwBodiesClone = bwBodies.clone();
	findContours(bwBodiesClone, allBodyContours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()
	size_t bodyContourPixelCount = 0;
	for (std::vector<std::vector<cv::Point> >::const_iterator iter = allBodyContours.begin(); iter != allBodyContours.end(); ++iter) {
		bodyContourPixelCount += iter->size();
	}

	// write these contours to file as a per-frame contour set
	if (contourFile) {
		bodyContourOffset = writeContour(allBodyContours);
	}

	// get wing areas
	//TODO: why don't we use smoothForeground?
	cv::Mat fgMedian;
	cvtColor(background - frame, fgMedian, CV_BGR2GRAY);
	fgMedian = removeVerticalWave(fgMedian);

	cv::Mat fgSaturatedWings = fgMedian & mask & ~bwBodies;
	size_t totalPixelCount = fgSaturatedWings.rows * fgSaturatedWings.cols;
	size_t pixelsToSaturate = 3 * bodyContourPixelCount;	// doSegmentation.m in MATLAB tracker uses factor 3
	if (pixelsToSaturate == 0 || pixelsToSaturate >= totalPixelCount) {
		std::cerr << "warning: pixelsToSaturate (" << pixelsToSaturate << ") must be between 0 and totalPixelCount (" << totalPixelCount << ") ... skipping saturation step of wing segmentation!" << std::endl;
	} else {
		cv::Mat fgSaturatedWingsClone = fgSaturatedWings.clone();
		assert(fgSaturatedWingsClone.isContinuous());
		std::nth_element(fgSaturatedWingsClone.data, fgSaturatedWingsClone.data + (totalPixelCount - pixelsToSaturate), fgSaturatedWingsClone.data + totalPixelCount);
		unsigned char saturateAbove = *(fgSaturatedWingsClone.data + (totalPixelCount - pixelsToSaturate));
		fgSaturatedWings = stretch(fgSaturatedWings, 0, saturateAbove);
	}

	for (int row = 0; row != arenaContours.rows; ++row) {
		for (int col = 0; col != arenaContours.cols; ++col) {
			arenaContours.ptr<uchar>(row)[3*col] = fgSaturatedWings.ptr<uchar>(row)[col];
			arenaContours.ptr<uchar>(row)[3*col+1] = fgSaturatedWings.ptr<uchar>(row)[col];
			arenaContours.ptr<uchar>(row)[3*col+2] = fgSaturatedWings.ptr<uchar>(row)[col];
		}
	}

	cv::Mat bwWings;
	unsigned char wingThreshold = threshold(fgSaturatedWings, bwWings, 25, 255, cv::THRESH_OTSU);
	bwWings = bwWings | bwBodies;

	if (saveDebugImages) {
		imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwWings_after_thresholding.png", bwWings);
	}

	// remove legs
	//TODO: should this be made resolution-independent?
	morphologyEx(bwWings, bwWings, cv::MORPH_OPEN, cv::Mat(5, 5, CV_8UC1, 255));
	morphologyEx(bwWings, bwWings, cv::MORPH_CLOSE, cv::Mat(5, 5, CV_8UC1, 255));
	bwWings = bwWings | bwBodies;

	if (saveDebugImages) {
		imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwWings_after_legremoval.png", bwWings);
	}

	// remove wings that have no bodies by filling the wings using the bodies as seed
	bwWings = reconstruct((bwBodies & bwWings) > 0, bwWings > 0, 8);
	bwWings = bwWings * 255;

	if (saveDebugImages) {
		imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwWings_after_reconstruct.png", bwWings);
	}

	std::vector<std::vector<cv::Point> > wingContours;
	cv::Mat bwWingsClone = bwWings.clone();
	findContours(bwWingsClone, wingContours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()
	{
		std::vector<std::vector<cv::Point> > wingsToRemove;
		for (std::vector<std::vector<cv::Point> >::const_iterator iter = wingContours.begin(); iter != wingContours.end(); ++iter) {
			if (fewerThan6(*iter)) {
				wingsToRemove.push_back(*iter);
			}
		}
		wingContours.erase(std::remove_if(wingContours.begin(), wingContours.end(), fewerThan6), wingContours.end());	// otherwise, fitEllipse throws an error
		drawContours(bwWings, wingsToRemove, -1, cv::Scalar(0, 0, 0), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
		drawContours(bwBodies, wingsToRemove, -1, cv::Scalar(0, 0, 0), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	}

	// write these contours to file as a per-frame contour set
	if (contourFile) {
		wingContourOffset = writeContour(wingContours);
	}

	// if there are more wing regions than flyCount, remove the smallest ones
	//TODO: use cv::contourArea instead of borderpixelcount?
	if (wingContours.size() > getFlyCount()) {
		std::sort(wingContours.begin(), wingContours.end(), bigger<std::vector<cv::Point> >);
		std::vector<std::vector<cv::Point> > wingsToRemove(wingContours.begin() + getFlyCount(), wingContours.end());
		wingContours.erase(wingContours.begin() + getFlyCount(), wingContours.end());
		// here we fill the contours with black. labelling and using logical operations may be faster.
		drawContours(bwWings, wingsToRemove, -1, cv::Scalar(0, 0, 0), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
		drawContours(bwBodies, wingsToRemove, -1, cv::Scalar(0, 0, 0), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	}

	if (saveDebugImages) {
		imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwBodies_after_removal_of_smallest_wings.png", bwBodies);
		imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwWings_after_removal_of_smallest_wings.png", bwWings);
	}

	// we now have 0 or more (at most flyCount) wing regions, each one with at least 1 body

	cv::Mat wingIndexImage(bwWings.size(), CV_32SC1, cv::Scalar(-1));	// background is -1
	for (size_t wingIndex = 0; wingIndex != wingContours.size(); ++wingIndex) {
		drawContours(wingIndexImage, wingContours, wingIndex, cv::Scalar(wingIndex, wingIndex, wingIndex), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	}

	// calculate wing centroids now so we can assign bodies whose centroid is not within a wing (a very common case for true occlusions, which often form concave shapes) to the closest wing region
	//TODO: rather than calculating this and the wingIndexImage, we should calculate a voronoi diagram where each wingContour makes up one class
	std::vector<cv::Point> wingCentroids;
	for (size_t wingIndex = 0; wingIndex != wingContours.size(); ++wingIndex) {
		cv::Point sum = std::accumulate(wingContours[wingIndex].begin(), wingContours[wingIndex].end(), cv::Point());
		wingCentroids.push_back(cv::Point(sum.x / wingContours[wingIndex].size(), sum.y / wingContours[wingIndex].size()));
	}

	// there are some calls above that change bwBodies so we update allBodyContours here
	bwBodiesClone = bwBodies.clone();
	findContours(bwBodiesClone, allBodyContours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()

	std::vector<MergeableBodyContour> mergeableBodyContours;
	mergeableBodyContours.reserve(allBodyContours.size());
	for (size_t bodyIndex = 0; bodyIndex != allBodyContours.size(); ++bodyIndex) {
		MergeableBodyContour newContour;
		newContour.contour = allBodyContours[bodyIndex];
		cv::Point sum = std::accumulate(allBodyContours[bodyIndex].begin(), allBodyContours[bodyIndex].end(), cv::Point());
		cv::Point centroid(sum.x / allBodyContours[bodyIndex].size(), sum.y / allBodyContours[bodyIndex].size());
		newContour.centroid = centroid;
		newContour.wingIndex = wingIndexImage.at<int>(centroid);
		newContour.mergeable = true;
		newContour.split = false;
		if (newContour.wingIndex >= 0) {	// if the body part falls within a wing-region
			mergeableBodyContours.push_back(newContour);
		} else {	// find the closest wing region
			float smallestDistance = std::numeric_limits<float>::max();
			size_t closestWing = 0;
			bool otherWingFound = false;
			for (size_t wingIndex = 0; wingIndex != wingCentroids.size(); ++wingIndex) {
				int dx = wingCentroids[wingIndex].x - newContour.centroid.x;
				int dy = wingCentroids[wingIndex].y - newContour.centroid.y;
				float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
				if (distance < smallestDistance) {
					smallestDistance = distance;
					closestWing = wingIndex;
					otherWingFound = true;
				}
			}
			if (otherWingFound) {
				newContour.wingIndex = closestWing;
				mergeableBodyContours.push_back(newContour);
			}
			// else there's no wing region and we drop the body
		}
	}

	std::vector<Fly> fliesInThisFrame;

	std::vector<SplittableWingContour> splittableWingContours(wingContours.size());

	// merge the (globally) smallest bodies to the closest one in the same wing, until there are only flyCount bodies left
	while (mergeableBodyContours.size() > getFlyCount()) {
		size_t smallestSize = std::numeric_limits<size_t>::max();
		size_t smallestBody = 0;
		for (size_t bodyIndex = 0; bodyIndex != mergeableBodyContours.size(); ++bodyIndex) {
			if (mergeableBodyContours[bodyIndex].mergeable &&
				mergeableBodyContours[bodyIndex].contour.size() < smallestSize) {
				smallestSize = mergeableBodyContours[bodyIndex].contour.size();
				smallestBody = bodyIndex;
			}
		}

		std::vector<std::pair<float, size_t> > distances;	// distance, index into mergeableBodyContours
		for (size_t bodyIndex = 0; bodyIndex != mergeableBodyContours.size(); ++bodyIndex) {
			if (bodyIndex != smallestBody &&	// don't merge a body to itself
				mergeableBodyContours[bodyIndex].wingIndex == mergeableBodyContours[smallestBody].wingIndex) {
				// we found another body contour in the same wing, now let's see if it's closer than the best one found so far
				int dx = mergeableBodyContours[bodyIndex].centroid.x - mergeableBodyContours[smallestBody].centroid.x;
				int dy = mergeableBodyContours[bodyIndex].centroid.y - mergeableBodyContours[smallestBody].centroid.y;
				float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
				distances.push_back(std::pair<float, size_t>(distance, bodyIndex));
			}
		}
		if (distances.empty()) {
			mergeableBodyContours[smallestBody].mergeable = false;
			continue;
		}
		std::sort(distances.begin(), distances.end());

		// merge smallestBody with closestBody that doesn't produce pixels outside the wing region after the merge
		bool couldMergeSmallest = false;
		for (size_t distancesIndex = 0; distancesIndex != distances.size(); ++distancesIndex) {
			size_t closestBody = distances[distancesIndex].second;
			std::vector<cv::Point> bodyContoursMerged;
			bodyContoursMerged.insert(bodyContoursMerged.end(), mergeableBodyContours[smallestBody].contour.begin(), mergeableBodyContours[smallestBody].contour.end());
			bodyContoursMerged.insert(bodyContoursMerged.end(), mergeableBodyContours[closestBody].contour.begin(), mergeableBodyContours[closestBody].contour.end());
			cv::Mat bodyContoursMergedAsMat(bodyContoursMerged);
			std::vector<cv::Point> bodyContoursHull;
			convexHull(bodyContoursMergedAsMat, bodyContoursHull);
			cv::Mat mergedBodiesInThisWing(bwBodies.size(), CV_8UC1, cv::Scalar(0));
			fillConvexPoly(mergedBodiesInThisWing, &bodyContoursHull[0], bodyContoursHull.size(), cv::Scalar(255, 255, 255));
/*
			// see if any of the pixels of the merged region are outside the wing region
			cv::Mat thisWingArea = cv::Mat(bwBodies.size(), CV_8UC1, cv::Scalar(0));
			drawContours(thisWingArea, wingContours, mergeableBodyContours[smallestBody].wingIndex, cv::Scalar(255, 255, 255), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
			cv::Mat outsideOfWing = mergedBodiesInThisWing & ~thisWingArea;
			bool anyOutsideOfWing = false;
			for (size_t i = 0; i != outsideOfWing.rows * outsideOfWing.cols; ++i) {
				if (*(outsideOfWing.data + i)) {
					anyOutsideOfWing = true;
					break;
				}
			}
			if (anyOutsideOfWing) {
				continue;	// try merging with the next-closest
			}
*/
			std::vector<std::vector<cv::Point> > mergedBodyContours;
			cv::Mat mergedBodiesInThisWingClone = mergedBodiesInThisWing.clone();
			findContours(mergedBodiesInThisWingClone, mergedBodyContours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()
			// sanity check
			if (mergedBodyContours.size() != 1) {
				throw std::logic_error("there must be exactly one body region after merging");
			}
			MergeableBodyContour mergedBody;
			mergedBody.contour = mergedBodyContours[0];
			cv::Point sum = mergeableBodyContours[smallestBody].centroid + mergeableBodyContours[closestBody].centroid;
			mergedBody.centroid = cv::Point(sum.x / 2, sum.y / 2);
			mergedBody.wingIndex = mergeableBodyContours[smallestBody].wingIndex;
			mergedBody.mergeable = true;
			mergedBody.split = mergeableBodyContours[smallestBody].split || mergeableBodyContours[closestBody].split;

			//TODO: can we use std::list instead of std::vector for mergeableBodyContours to make this more efficient?
			if (smallestBody < closestBody) {
				mergeableBodyContours[smallestBody] = mergedBody;
				mergeableBodyContours.erase(mergeableBodyContours.begin() + closestBody);
			} else {
				mergeableBodyContours[closestBody] = mergedBody;
				mergeableBodyContours.erase(mergeableBodyContours.begin() + smallestBody);
			}
			couldMergeSmallest = true;
			break;
		}
		if (!couldMergeSmallest) {
			std::cerr << "missegmentation: body could not be merged in arena " << id << ", frame " << videoFrameNumber << std::endl;
			missegmented = true;
			goto DONE_TRACKING_THIS_FRAME;
		}
	}

	// quick hack: if there are 2 body regions in the same wing and it's a Small-Large missegmentation, merge that as well
	if (fullyMergeMissegmentations) {
		if (getFlyCount() == 2 && mergeableBodyContours.size() == 2 && mergeableBodyContours[0].wingIndex == mergeableBodyContours[1].wingIndex) {
			bool fly0Small = false;
			bool fly0Large = false;
			bool fly1Small = false;
			bool fly1Large = false;

			const float minArea = minFlyBodySizeSquareMillimeter * pixelPerMillimeter * pixelPerMillimeter;
			const float maxArea = maxFlyBodySizeSquareMillimeter * pixelPerMillimeter * pixelPerMillimeter;

			if (mergeableBodyContours[0].contour.size() < 6) {	// fitEllipse would throw an error
				fly0Small = true;
			} else {
				cv::Mat bodyContourAsMat(mergeableBodyContours[0].contour);
				cv::RotatedRect bodyEllipseBB = fitEllipse(bodyContourAsMat);
				float e = eccentricity(std::max(bodyEllipseBB.size.width, bodyEllipseBB.size.height), std::min(bodyEllipseBB.size.width, bodyEllipseBB.size.height));
				float eccentricityCorrected = e / (1 + exp(-5 * (e - 0.5)));
				float area = (bodyEllipseBB.size.width / 2) * (bodyEllipseBB.size.height / 2) * CV_PI * sqrt(1 - eccentricityCorrected * eccentricityCorrected);
				fly0Small = area < minArea;
				fly0Large = area > maxArea;
			}

			if (mergeableBodyContours[1].contour.size() < 6) {	// fitEllipse would throw an error
				fly1Small = true;
			} else {
				cv::Mat bodyContourAsMat(mergeableBodyContours[1].contour);
				cv::RotatedRect bodyEllipseBB = fitEllipse(bodyContourAsMat);
				float e = eccentricity(std::max(bodyEllipseBB.size.width, bodyEllipseBB.size.height), std::min(bodyEllipseBB.size.width, bodyEllipseBB.size.height));
				float eccentricityCorrected = e / (1 + exp(-5 * (e - 0.5)));
				float area = (bodyEllipseBB.size.width / 2) * (bodyEllipseBB.size.height / 2) * CV_PI * sqrt(1 - eccentricityCorrected * eccentricityCorrected);
				fly1Small = area < minArea;
				fly1Large = area > maxArea;
			}

			if ((fly0Small && fly1Large) || (fly0Large && fly1Small)) {
				size_t smallestBody = fly0Small ? 0 : 1;
				size_t closestBody = fly0Small ? 1 : 0;
				std::vector<cv::Point> bodyContoursMerged;
				bodyContoursMerged.insert(bodyContoursMerged.end(), mergeableBodyContours[smallestBody].contour.begin(), mergeableBodyContours[smallestBody].contour.end());
				bodyContoursMerged.insert(bodyContoursMerged.end(), mergeableBodyContours[closestBody].contour.begin(), mergeableBodyContours[closestBody].contour.end());
				cv::Mat bodyContoursMergedAsMat(bodyContoursMerged);
				std::vector<cv::Point> bodyContoursHull;
				convexHull(bodyContoursMergedAsMat, bodyContoursHull);
				cv::Mat mergedBodiesInThisWing(bwBodies.size(), CV_8UC1, cv::Scalar(0));
				fillConvexPoly(mergedBodiesInThisWing, &bodyContoursHull[0], bodyContoursHull.size(), cv::Scalar(255, 255, 255));

				std::vector<std::vector<cv::Point> > mergedBodyContours;
				cv::Mat mergedBodiesInThisWingClone = mergedBodiesInThisWing.clone();
				findContours(mergedBodiesInThisWingClone, mergedBodyContours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()
				// sanity check
				if (mergedBodyContours.size() != 1) {
					throw std::logic_error("there must be exactly one body region after merging");
				}
				MergeableBodyContour mergedBody;
				mergedBody.contour = mergedBodyContours[0];
				cv::Point sum = mergeableBodyContours[smallestBody].centroid + mergeableBodyContours[closestBody].centroid;
				mergedBody.centroid = cv::Point(sum.x / 2, sum.y / 2);
				mergedBody.wingIndex = mergeableBodyContours[smallestBody].wingIndex;
				mergedBody.mergeable = true;
				mergedBody.split = mergeableBodyContours[smallestBody].split || mergeableBodyContours[closestBody].split;

				//TODO: can we use std::list instead of std::vector for mergeableBodyContours to make this more efficient?
				if (smallestBody < closestBody) {
					mergeableBodyContours[smallestBody] = mergedBody;
					mergeableBodyContours.erase(mergeableBodyContours.begin() + closestBody);
				} else {
					mergeableBodyContours[closestBody] = mergedBody;
					mergeableBodyContours.erase(mergeableBodyContours.begin() + smallestBody);
				}
				std::cerr << "warning: Small-Large missegmentation merged in arena " << id << ", frame " << videoFrameNumber << "!" << std::endl;
			}
		}
	}

	// we now have 0 or more (at most flyCount) wing regions, each one with at least 1 body
	// also, there are at most flyCount bodies in total

	// boc
	if (!frames.empty() && getFlyCount() == 2) {	//TODO: this doesn't work for fly counts != 2 because of how the voronoi diagram is calculated
		const TrackedFrame& lastFrame = frames.back();
	
		// if this frame is an occlusion and the last frame is not, generate the voronoi diagram from the last frame's contours
		if (mergeableBodyContours.size() == 1 && !lastFrame.get_isOcclusionTouched()) {
			cv::Mat fly0BodyContoursBefore(frame.size(), CV_8UC1, cv::Scalar(0));
			drawContours(fly0BodyContoursBefore, lastFrame.fly(0).get_bodyContour(), -1, cv::Scalar(255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	
			cv::Mat fly1BodyContoursBefore(frame.size(), CV_8UC1, cv::Scalar(0));
			drawContours(fly1BodyContoursBefore, lastFrame.fly(1).get_bodyContour(), -1, cv::Scalar(255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	
			cv::Mat fly0Distance;
			distanceTransform(~fly0BodyContoursBefore, fly0Distance, CV_DIST_L2, 5);
			
			cv::Mat fly1Distance;
			distanceTransform(~fly1BodyContoursBefore, fly1Distance, CV_DIST_L2, 5);
	
			cv::Mat closest = fly0Distance > fly1Distance;
	
			std::vector<cv::Point> currentContour = mergeableBodyContours[0].contour;
			size_t lastId = 0;
			for (std::vector<cv::Point>::const_iterator iter = currentContour.begin(); iter != currentContour.end(); ++iter) {
				size_t currentId = (closest.ptr<uchar>(iter->y)[iter->x] == 0) ? 0 : 1;
				if (currentId != lastId || iter == currentContour.begin()) {
					// start a new segment
					bocContours[currentId].resize(bocContours[currentId].size() + 1);
				}
				bocContours[currentId].back().push_back(*iter);

				if (closest.ptr<uchar>(iter->y)[iter->x] == 0) {
					carry0.push_back(*iter);
				} else {
					carry1.push_back(*iter);
				}

				lastId = currentId;
			}
			// merge first and last segment if they belong to the same fly
			if (!currentContour.empty()) {
				size_t frontId = (closest.ptr<uchar>(currentContour.front().y)[currentContour.front().x] == 0) ? 0 : 1;
				size_t backId = (closest.ptr<uchar>(currentContour.back().y)[currentContour.back().x] == 0) ? 0 : 1;
				if (frontId == backId && bocContours[frontId].size() > 1) {
					bocContours[frontId].front().insert(bocContours[frontId].front().begin(), bocContours[frontId].back().begin(), bocContours[frontId].back().end());
					bocContours[frontId].pop_back();
				}
			}
		}
		
		// if both this and the last frame are occlusions, keep carrying
		if (mergeableBodyContours.size() == 1 && lastFrame.get_isOcclusionTouched()) {
			if (lastFrame.carry0.empty()) {
				bocContours[1].push_back(mergeableBodyContours[0].contour);
				carry1 = mergeableBodyContours[0].contour;
			} else if (lastFrame.carry1.empty()) {
				bocContours[0].push_back(mergeableBodyContours[0].contour);
				carry0 = mergeableBodyContours[0].contour;
			} else {
				cv::Mat fly0BodyContoursBefore(frame.size(), CV_8UC1, cv::Scalar(0));
				std::vector<std::vector<cv::Point> > fly0ContoursToDraw;
				fly0ContoursToDraw.push_back(lastFrame.carry0);
				drawContours(fly0BodyContoursBefore, fly0ContoursToDraw, -1, cv::Scalar(255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	
				cv::Mat fly1BodyContoursBefore(frame.size(), CV_8UC1, cv::Scalar(0));
				std::vector<std::vector<cv::Point> > fly1ContoursToDraw;
				fly1ContoursToDraw.push_back(lastFrame.carry1);
				drawContours(fly1BodyContoursBefore, fly1ContoursToDraw, -1, cv::Scalar(255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	
				cv::Mat fly0Distance;
				distanceTransform(~fly0BodyContoursBefore, fly0Distance, CV_DIST_L2, 5);
				
				cv::Mat fly1Distance;
				distanceTransform(~fly1BodyContoursBefore, fly1Distance, CV_DIST_L2, 5);
	
				cv::Mat closest = fly0Distance > fly1Distance;
	
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly0BodyContoursBefore.png", fly0BodyContoursBefore);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly1BodyContoursBefore.png", fly1BodyContoursBefore);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly0Distance.png", fly0Distance);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly1Distance.png", fly1Distance);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_closest.png", closest);
				
				std::vector<cv::Point> currentContour = mergeableBodyContours[0].contour;
				size_t lastId = 0;
				for (std::vector<cv::Point>::const_iterator iter = currentContour.begin(); iter != currentContour.end(); ++iter) {
					size_t currentId = (closest.ptr<uchar>(iter->y)[iter->x] == 0) ? 0 : 1;
					if (currentId != lastId || iter == currentContour.begin()) {
						// start a new segment
						bocContours[currentId].resize(bocContours[currentId].size() + 1);
					}
					bocContours[currentId].back().push_back(*iter);

					if (closest.ptr<uchar>(iter->y)[iter->x] == 0) {
						carry0.push_back(*iter);
					} else {
						carry1.push_back(*iter);
					}

					lastId = currentId;
				}
				// merge first and last segment if they belong to the same fly
				if (!currentContour.empty()) {
					size_t frontId = (closest.ptr<uchar>(currentContour.front().y)[currentContour.front().x] == 0) ? 0 : 1;
					size_t backId = (closest.ptr<uchar>(currentContour.back().y)[currentContour.back().x] == 0) ? 0 : 1;
					if (frontId == backId && bocContours[frontId].size() > 1) {
						bocContours[frontId].front().insert(bocContours[frontId].front().begin(), bocContours[frontId].back().begin(), bocContours[frontId].back().end());
						bocContours[frontId].pop_back();
					}
				}
			}
		}
	
		// if the last frame was an occlusion, but this one is not, calculate the score
		if (mergeableBodyContours.size() == 2 && lastFrame.get_isOcclusionTouched()) {
			if (!lastFrame.carry0.empty() && !lastFrame.carry1.empty() ) {
				cv::Mat fly0BodyContoursBefore(frame.size(), CV_8UC1, cv::Scalar(0));
				std::vector<std::vector<cv::Point> > fly0ContoursToDraw;
				fly0ContoursToDraw.push_back(lastFrame.carry0);
				drawContours(fly0BodyContoursBefore, fly0ContoursToDraw, -1, cv::Scalar(255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	
				cv::Mat fly1BodyContoursBefore(frame.size(), CV_8UC1, cv::Scalar(0));
				std::vector<std::vector<cv::Point> > fly1ContoursToDraw;
				fly1ContoursToDraw.push_back(lastFrame.carry1);
				drawContours(fly1BodyContoursBefore, fly1ContoursToDraw, -1, cv::Scalar(255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	
				cv::Mat fly0Distance;
				distanceTransform(~fly0BodyContoursBefore, fly0Distance, CV_DIST_L2, 5);
				
				cv::Mat fly1Distance;
				distanceTransform(~fly1BodyContoursBefore, fly1Distance, CV_DIST_L2, 5);
	
				cv::Mat closest = fly0Distance > fly1Distance;
	
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly0BodyContoursBefore.png", fly0BodyContoursBefore);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly1BodyContoursBefore.png", fly1BodyContoursBefore);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly0Distance.png", fly0Distance);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_fly1Distance.png", fly1Distance);
				//imwrite(global::outDir + "/" + stringify(getId()) + "_" + stringify(videoFrameNumber) + "_closest.png", closest);
				
				size_t map0To0 = 0;
				size_t map0To1 = 0;
				size_t map1To0 = 0;
				size_t map1To1 = 0;
				
				std::vector<cv::Point> currentContour = mergeableBodyContours[0].contour;
				for (std::vector<cv::Point>::const_iterator iter = currentContour.begin(); iter != currentContour.end(); ++iter) {
					if (closest.ptr<uchar>(iter->y)[iter->x] == 0) {
						++map0To0;
					} else {
						++map1To0;
					}
				}
	
				currentContour = mergeableBodyContours[1].contour;
				for (std::vector<cv::Point>::const_iterator iter = currentContour.begin(); iter != currentContour.end(); ++iter) {
					if (closest.ptr<uchar>(iter->y)[iter->x] == 0) {
						++map0To1;
					} else {
						++map1To1;
					}
				}
				
				// calculate boc score and write it to the affected frames
				bocScoreCalculated =
					(static_cast<float>(map0To0 + map1To1) - static_cast<float>(map0To1 + map1To0)) /
					static_cast<float>(map0To0 + map1To1 + map0To1 + map1To0);
			}
		}
	}

	// initialize splittableWingContours
	for (size_t wingIndex = 0; wingIndex != wingContours.size(); ++wingIndex) {
		splittableWingContours[wingIndex].mask = cv::Mat(bwBodies.size(), CV_8UC1, cv::Scalar(0));
		drawContours(splittableWingContours[wingIndex].mask, wingContours, wingIndex, cv::Scalar(255, 255, 255), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
		splittableWingContours[wingIndex].split = false;
	}
	for (size_t bodyIndex = 0; bodyIndex != mergeableBodyContours.size(); ++bodyIndex) {
		splittableWingContours[mergeableBodyContours[bodyIndex].wingIndex].bodyIndexes.insert(bodyIndex);
	}

	if (splitBodies) {
		// split the bodies within occlusions using the longest bocContour segment as seed to fill the combined body area
		if (mergeableBodyContours.size() == 1 && !carry0.empty() && !carry1.empty()) {
			size_t longestSegment0 = 0;
			for (size_t segmentNumber = 0; segmentNumber != bocContours[0].size(); ++segmentNumber) {
				if (bocContours[0][segmentNumber].size() > bocContours[0][longestSegment0].size()) {
					longestSegment0 = segmentNumber;
				}
			}

			size_t longestSegment1 = 0;
			for (size_t segmentNumber = 0; segmentNumber != bocContours[1].size(); ++segmentNumber) {
				if (bocContours[1][segmentNumber].size() > bocContours[1][longestSegment1].size()) {
					longestSegment1 = segmentNumber;
				}
			}

			cv::Mat mask(bwBodies.size(), CV_8UC1, cv::Scalar(0));
			std::vector<std::vector<cv::Point> > bodyContoursToDraw;
			bodyContoursToDraw.push_back(mergeableBodyContours[0].contour);
			drawContours(mask, bodyContoursToDraw, 0, cv::Scalar(255, 255, 255), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());

			std::vector<cv::Mat> markers;
			markers.push_back(cv::Mat(bwBodies.size(), CV_8UC1, cv::Scalar(0)));
			drawContours(markers.back(), bocContours[0], longestSegment0, cv::Scalar(255, 255, 255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
			markers.push_back(cv::Mat(bwBodies.size(), CV_8UC1, cv::Scalar(0)));
			drawContours(markers.back(), bocContours[1], longestSegment1, cv::Scalar(255, 255, 255), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());

			std::vector<cv::Mat> reconstructed = parallelReconstruct(markers, mask);

			std::vector<std::vector<cv::Point> > splitContours0;
			cv::Mat r0Clone = reconstructed[0].clone();
			findContours(r0Clone, splitContours0, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()
			std::vector<std::vector<cv::Point> > splitContours1;
			cv::Mat r1Clone = reconstructed[1].clone();
			findContours(r1Clone, splitContours1, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()

			if (splitContours0.size() != 1 || splitContours1.size() != 1) {
				std::cerr << "warning: failed to split bodies in arena " << id << ", frame " << videoFrameNumber << ": splitContours0 (size " << splitContours0.size() << ") and splitContours1 (size " << splitContours1.size() << ") should have size 1 each ... skipping!" << std::endl;
			} else {
				MergeableBodyContour new0 = mergeableBodyContours.back();
				MergeableBodyContour new1 = mergeableBodyContours.back();
				mergeableBodyContours.pop_back();
				new0.contour = splitContours0[0];
				new1.contour = splitContours1[0];
				new0.split = true;
				new1.split = true;

				cv::Point sum0 = std::accumulate(new0.contour.begin(), new0.contour.end(), cv::Point());
				new0.centroid = cv::Point(sum0.x / new0.contour.size(), sum0.y / new0.contour.size());

				cv::Point sum1 = std::accumulate(new1.contour.begin(), new1.contour.end(), cv::Point());
				new1.centroid = cv::Point(sum1.x / new1.contour.size(), sum1.y / new1.contour.size());

				mergeableBodyContours.push_back(new0);
				mergeableBodyContours.push_back(new1);
				splittableWingContours[new1.wingIndex].bodyIndexes.insert(mergeableBodyContours.size() - 1);
			}
		}
	}

	if (splitWings) {
		// split each wing with more than one body
		for (size_t wingIndex = 0; wingIndex != splittableWingContours.size(); ++wingIndex) {
			size_t bodyCount = splittableWingContours[wingIndex].bodyIndexes.size();
			if (bodyCount > 1) {
				std::vector<cv::Mat> markers;
				markers.reserve(bodyCount);
				for (size_t markerIndex = 0; markerIndex != bodyCount; ++markerIndex) {
					std::set<size_t>::iterator bodyIndexIter = splittableWingContours[wingIndex].bodyIndexes.begin();
					std::advance(bodyIndexIter, markerIndex);
					size_t bodyIndex = *bodyIndexIter;
					markers.push_back(cv::Mat(bwBodies.size(), CV_8UC1, cv::Scalar(0)));
					std::vector<std::vector<cv::Point> > bodyContoursToDraw;
					bodyContoursToDraw.push_back(mergeableBodyContours[bodyIndex].contour);
					drawContours(markers.back(), bodyContoursToDraw, 0, cv::Scalar(255, 255, 255), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
				}
				std::vector<cv::Mat> reconstructed = parallelReconstruct(markers, splittableWingContours[wingIndex].mask);
				for (size_t reconIndex = 0; reconIndex != reconstructed.size(); ++reconIndex) {
					// add a new wing to the end of splittableWingContours and make its body refer to it
					std::set<size_t>::iterator bodyIndexIter = splittableWingContours[wingIndex].bodyIndexes.begin();
					std::advance(bodyIndexIter, reconIndex);
					size_t bodyIndex = *bodyIndexIter;
					mergeableBodyContours[bodyIndex].wingIndex = splittableWingContours.size();
					splittableWingContours.resize(splittableWingContours.size() + 1);
					splittableWingContours.back().mask = reconstructed[reconIndex];
					splittableWingContours.back().bodyIndexes.insert(bodyIndex);
					splittableWingContours.back().split = true;
				}
			}
		}

		// from the new wings in splittableWingContours, derive the actual contours and add them to wingContours
		for (size_t i = wingContours.size(); i != splittableWingContours.size(); ++i) {
			std::vector<std::vector<cv::Point> > splitWingContours;
			cv::Mat wingMaskClone = splittableWingContours[i].mask.clone();
			findContours(wingMaskClone, splitWingContours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	// findContours changes the image, hence the .clone()
			// sanity check
			if (splitWingContours.size() != 1) {
				if (saveDebugImages) {
					imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwBodies.png", bwBodies);
					for (size_t debugIndex = 0; debugIndex != mergeableBodyContours.size(); ++debugIndex) {
						cv::Mat mergedBodiesInThisWing(bwBodies.size(), CV_8UC1, cv::Scalar(0));
						std::vector<std::vector<cv::Point> > bodyContoursToDraw;
						bodyContoursToDraw.push_back(mergeableBodyContours[debugIndex].contour);
						drawContours(mergedBodiesInThisWing, bodyContoursToDraw, 0, cv::Scalar(255, 255, 255), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
						imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bodyMask_" + stringify(debugIndex) + ".png", mergedBodiesInThisWing);
					}
					for (size_t debugIndex = 0; debugIndex != splittableWingContours.size(); ++debugIndex) {
						imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_wingMask_" + stringify(debugIndex) + ".png", splittableWingContours[debugIndex].mask);
					}
				}
				//throw std::logic_error("there must be exactly one wing region after the split");
				std::cerr << "missegmentation: more than one wing region after the split in arena " << id << ", frame " << videoFrameNumber << std::endl;
				missegmented = true;
				goto DONE_TRACKING_THIS_FRAME;
			}
			wingContours.push_back(splitWingContours[0]);
		}
	}

	for (size_t bodyIndex = 0; bodyIndex != mergeableBodyContours.size(); ++bodyIndex) {
		try {
			std::vector<std::vector<cv::Point> > finalBodyContour(1, mergeableBodyContours[bodyIndex].contour);
			std::vector<std::vector<cv::Point> > finalWingContour(1, wingContours[mergeableBodyContours[bodyIndex].wingIndex]);

			// write contours to the file
			size_t thisFlyBodyContourOffset = 0;
			size_t thisFlyWingContourOffset = 0;
			size_t thisFlyBocContourOffset = 0;
			if (contourFile) {
				thisFlyBodyContourOffset = writeContour(finalBodyContour);
				thisFlyWingContourOffset = writeContour(finalWingContour);
				if (!bocContours[bodyIndex].empty()) {
					thisFlyBocContourOffset = writeContour(bocContours[bodyIndex]);
				}
			}

			// create the body mask
			cv::Mat mergedBodiesInThisWing(bwBodies.size(), CV_8UC1, cv::Scalar(0));
			drawContours(mergedBodiesInThisWing, finalBodyContour, 0, cv::Scalar(255, 255, 255), -1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());

			Fly fly(
				frame,
				smoothForeground,
				mergedBodiesInThisWing,
				finalBodyContour,
				mergeableBodyContours[bodyIndex].split,
				thisFlyBodyContourOffset,
				thisFlyBocContourOffset,
				finalWingContour,
				thisFlyWingContourOffset
			);
			fliesInThisFrame.push_back(fly);
			drawFly(fliesInThisFrame.back(), finalBodyContour, wingContours, arenaContours);
		} catch (std::runtime_error& e) {
			// this frame will be occluded
			std::cerr << "warning: " << e.what() << std::endl;
		}
	}

	if (saveDebugImages) {
		imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwBodies.png", bwBodies);
		imwrite(global::outDir + "/" + getId() + "/" + stringify(videoFrameNumber) + "_bwWings.png", bwWings);
	}

DONE_TRACKING_THIS_FRAME:
	// check if this frame has been missegmented
	float minArea = minFlyBodySizeSquareMillimeter * pixelPerMillimeter * pixelPerMillimeter;
	float maxArea = maxFlyBodySizeSquareMillimeter * pixelPerMillimeter * pixelPerMillimeter;
	if (fliesInThisFrame.size() == getFlyCount()) {
		for (size_t flyNumber = 0; flyNumber != fliesInThisFrame.size(); ++flyNumber) {
			float bodyArea = fliesInThisFrame[flyNumber].get_bodyAreaEccentricityCorrectedTracked();
			bool bodySplit = fliesInThisFrame[flyNumber].get_bodySplit();
			if (!bodySplit && (bodyArea < minArea || bodyArea > maxArea)) {
				missegmented = true;
				break;
			}
		}
	}

	TrackedFrame thisFrame(
		frame.size(),
		sourceFrameRate,
		videoFrameNumber,
		frames.size(),
		1.0f * videoFrameNumber / videoFrameTotalCount,
		1.0f * frames.size() / trackFrameTotalCount,
		fliesInThisFrame,
		getFlyCount(),
		missegmented,
		bodyThreshold,
		wingThreshold,
		bodyContourOffset,
		wingContourOffset
	);
	if (!missegmented) {
		thisFrame.carry0 = carry0;
		thisFrame.carry1 = carry1;

		if (bocScoreCalculated) {
			// spread it back throughout this occlusion
			for (std::vector<TrackedFrame>::reverse_iterator iter = frames.rbegin(); iter != frames.rend(); ++iter) {
				if (!iter->get_isOcclusionTouched() && !iter->get_isMissegmented()) {
					break;
				}
				iter->bocScore = bocScoreCalculated;
			}
		}
	}

	if (!frames.empty()) {
		thisFrame.rearrangeFlies(frames.back());	// affects non-occluded frames following another non-occluded frame
		frames.back().eraseContours();	// frees some memory
	}

	frames.push_back(thisFrame);
}

void Arena::normalizeTrackingData()
{
	if (contourFile) {
		contourFile->close();
	}

	if (smoothHistogramFile) {
		smoothHistogramFile->close();
	}

	// reserve memory
	frameAttributes.reserve(getFrameCount());
	flyAttributes.resize(getFlyCount());
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		flyAttributes[flyNumber].reserve(getFrameCount());
	}
	pairAttributes.resize(getFlyCount());
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		pairAttributes[activeFly].resize(getFlyCount());
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			pairAttributes[activeFly][passiveFly].reserve(getFrameCount());
		}
	}

	// copy tracking frame attributes
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		frameAttributes.appendFrame(frames[frameNumber]);
	}

	const Attribute<MyBool>& isOcclusionTouched = frameAttributes.getFilled<MyBool>("isOcclusionTouched");

	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (isOcclusionTouched[frameNumber]) {
			for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
				if (flyNumber < frames[frameNumber].flyCount()) {
					flyAttributes[flyNumber].appendFly(frames[frameNumber].fly(flyNumber));
				} else {
					flyAttributes[flyNumber].appendEmpty();
				}
			}
		} else { // not occluded because of touching flies
			for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
				flyAttributes[flyNumber].appendFly(frames[frameNumber].fly(flyNumber));
			}
		}
	}
	
	// free some memory
	std::vector<TrackedFrame> emptyFrames;
	frames.swap(emptyFrames);
}

void Arena::prepareInterpolation()
{
	const Attribute<MyBool>& isOcclusionTouched = frameAttributes.getFilled<MyBool>("isOcclusionTouched");
	const Attribute<MyBool>& isMissegmented = frameAttributes.getFilled<MyBool>("isMissegmented");

	Attribute<MyBool>& isOcclusion = frameAttributes.getEmpty<MyBool>("isOcclusion");
	Attribute<MyBool>& isMissegmentedUnmerged = frameAttributes.getEmpty<MyBool>("isMissegmentedUnmerged");
	Attribute<MyBool>& interpolated = frameAttributes.getEmpty<MyBool>("interpolated");
	Attribute<MyBool>& interpolatedAbsolutely = frameAttributes.getEmpty<MyBool>("interpolatedAbsolutely");
	Attribute<MyBool>& interpolatedRelatively = frameAttributes.getEmpty<MyBool>("interpolatedRelatively");

	// create isOcclusion by reconstructing
	std::vector<MyBool> isOcclusionReconstructionMask(getFrameCount());
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		isOcclusionReconstructionMask[frameNumber] = isMissegmented[frameNumber] || isOcclusionTouched[frameNumber];
	}
	isOcclusion.getData() = reconstruct(isOcclusionTouched.getData(), isOcclusionReconstructionMask);

	// missegmentations that are not part of occlusions
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		isMissegmentedUnmerged.push_back(isMissegmented[frameNumber] && !isOcclusion[frameNumber]);
	}

	// automatically fix sequences of missegmentations with
	// |pos| > 0.1 and frames <= 5 (actually, duration <= 0.2s)
	// |pos| > 0.2 and frames <= 8 (actually, duration <= 0.32s)
	// and make the remaining ones occlusions
	if (getFlyCount() == 2) {	//TODO: make this work for an arbitrary number of flies
		const Attribute<Vf2>& fly0Centroid = flyAttributes[0].getFilled<Vf2>("bodyCentroidTracked");
		const Attribute<Vf2>& fly1Centroid = flyAttributes[1].getFilled<Vf2>("bodyCentroidTracked");

		std::vector<bool> swapMask(getFrameCount());
		for (size_t missegmentedCount = 0, frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (isMissegmented[frameNumber]) {
				++missegmentedCount;
			} else if (missegmentedCount) {	// first frame after a sequence of missegmentations
				if (missegmentedCount < frameNumber) {	// there is a properly segmented frame before
					size_t frameNumberBefore = frameNumber - (missegmentedCount + 1);
					// get the pos score
					float firstToFirst = (fly0Centroid[frameNumberBefore] - fly0Centroid[frameNumber]).norm();
					float firstToSecond = (fly0Centroid[frameNumberBefore] - fly1Centroid[frameNumber]).norm();
					float secondToFirst = (fly1Centroid[frameNumberBefore] - fly0Centroid[frameNumber]).norm();
					float secondToSecond = (fly1Centroid[frameNumberBefore] - fly1Centroid[frameNumber]).norm();
					float straightDistance = firstToFirst + secondToSecond;
					float swappedDistance = firstToSecond + secondToFirst;
					float score = (swappedDistance - straightDistance) / (straightDistance + swappedDistance);

					if ((missegmentedCount / sourceFrameRate <= 0.2 && std::abs(score) > 0.1) ||
						(missegmentedCount / sourceFrameRate <= 0.32 && std::abs(score) > 0.2)) {
						// fix automatically
						if (score < 0) {
							swapMask[frameNumber] = true;	// here we mark the beginning of a swap only
						}
					} else {
						// make it an occlusion
						for (size_t f = frameNumber - 1; f >= frameNumber - missegmentedCount; --f) {
							isOcclusion[f] = true;
						}
					}
				}
				missegmentedCount = 0;
			}
		}

		// only the beginning of a swap has been marked...time to fill the swapMask properly
		bool isSwapped = false;
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (swapMask[frameNumber]) {
				isSwapped = !isSwapped;
			}
			swapMask[frameNumber] = isSwapped;
		}

		//frameAttributes.swap(swapMask);	//TODO: this changes scores and idPermutation...we don't want that!
		flyAttributes[0].swap(flyAttributes[1], swapMask);
		//TODO: what about pairAttributes?! FOR NOW they're all derived and calculated later, so it's OK!
	}

	// create interpolatedAbsolutely by reconstructing
	std::vector<MyBool> interpolatedAbsolutelyReconstructionMarker(getFrameCount());
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		interpolatedAbsolutelyReconstructionMarker[frameNumber] = isMissegmented[frameNumber] && isOcclusion[frameNumber];
	}
	interpolatedAbsolutely.getData() = reconstruct(interpolatedAbsolutelyReconstructionMarker, isOcclusion.getData());

	// add the unmerged missegmentations to interpolatedAbsolutely
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		interpolatedAbsolutely[frameNumber] = interpolatedAbsolutely[frameNumber] || isMissegmentedUnmerged[frameNumber];
	}

	// occlusions that are not interpolated absolutely can be interpolated relatively
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		interpolatedRelatively.push_back(!interpolatedAbsolutely[frameNumber] && isOcclusion[frameNumber]);
	}

	// remove frames in the beginning or end from interpolatedAbsolutely and interpolatedRelatively
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (!interpolatedRelatively[frameNumber]) {
			break;
		}
		interpolatedRelatively[frameNumber] = false;
	}
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (!interpolatedAbsolutely[frameNumber]) {
			break;
		}
		interpolatedAbsolutely[frameNumber] = false;
	}
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (!interpolatedRelatively[getFrameCount() - 1 - frameNumber]) {
			break;
		}
		interpolatedRelatively[getFrameCount() - 1 - frameNumber] = false;
	}
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (!interpolatedAbsolutely[getFrameCount() - 1 - frameNumber]) {
			break;
		}
		interpolatedAbsolutely[getFrameCount() - 1 - frameNumber] = false;
	}

	// interpolated
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		interpolated.push_back(interpolatedAbsolutely[frameNumber] || interpolatedRelatively[frameNumber]);
	}
}

void Arena::buildSequenceMaps()
{
	const Attribute<MyBool>& isOcclusion = frameAttributes.getFilled<MyBool>("isOcclusion");

	// build the occlusionMap
	bool previousFrameOccluded = false;
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (isOcclusion[frameNumber]) {
			if (!previousFrameOccluded) {
				occlusionMap.begin.push_back(frameNumber);
			}
			previousFrameOccluded = true;
		} else { // non-occluded frame
			if (previousFrameOccluded) {
				occlusionMap.end.push_back(frameNumber);
			}
			previousFrameOccluded = false;
		}
	}
	// if it's ending in an occlusion
	if (previousFrameOccluded) {
		occlusionMap.end.push_back(getFrameCount());
	}

	if (occlusionMap.begin.size() != occlusionMap.end.size()) {
		throw std::logic_error("occlusionMap is inconsistent");
	}
}

void Arena::detectMissegmentations(float minFlyBodySizeSquareMillimeter, float maxFlyBodySizeSquareMillimeter)
{
	std::vector<const Attribute<float>*> bodyAreas;
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		bodyAreas.push_back(&flyAttributes[flyNumber].getFilled<float>("bodyAreaEccentricityCorrectedTracked"));
	}

	const Attribute<MyBool>& isOcclusionTouched = frameAttributes.getFilled<MyBool>("isOcclusionTouched");

	Attribute<MyBool>& isMissegmentedSS = frameAttributes.getEmpty<MyBool>("isMissegmentedSS");
	Attribute<MyBool>& isMissegmentedS = frameAttributes.getEmpty<MyBool>("isMissegmentedS");
	Attribute<MyBool>& isMissegmentedSL = frameAttributes.getEmpty<MyBool>("isMissegmentedSL");
	Attribute<MyBool>& isMissegmentedL = frameAttributes.getEmpty<MyBool>("isMissegmentedL");
	Attribute<MyBool>& isMissegmentedLL = frameAttributes.getEmpty<MyBool>("isMissegmentedLL");

	isMissegmentedSS.resize(getFrameCount());
	isMissegmentedS.resize(getFrameCount());
	isMissegmentedSL.resize(getFrameCount());
	isMissegmentedL.resize(getFrameCount());
	isMissegmentedLL.resize(getFrameCount());

	float minArea = minFlyBodySizeSquareMillimeter * pixelPerMillimeter * pixelPerMillimeter;
	float maxArea = maxFlyBodySizeSquareMillimeter * pixelPerMillimeter * pixelPerMillimeter;

	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (isOcclusionTouched[frameNumber]) {
			continue;
		}

		{
			bool someTooSmall = false;
			bool someJustRight = false;
			bool someTooLarge = false;
			for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
				const float thisBodyArea = (*bodyAreas[flyNumber])[frameNumber];
				if (thisBodyArea < minArea) {
					someTooSmall = true;
				} else if (thisBodyArea > maxArea) {
					someTooLarge = true;
				} else {
					someJustRight = true;
				}
			}
			isMissegmentedSS[frameNumber] = someTooSmall && !someJustRight && !someTooLarge;	// all flies too small
			isMissegmentedS[frameNumber] = someTooSmall && someJustRight && !someTooLarge;	// some (but not all) flies too small, all others okay
			isMissegmentedSL[frameNumber] = someTooSmall && someTooLarge;	// some flies too small, some flies too large
			isMissegmentedL[frameNumber] = !someTooSmall && someJustRight && someTooLarge;	// some (but not all) flies too large, all others okay
			isMissegmentedLL[frameNumber] = !someTooSmall && !someJustRight && someTooLarge;	// all flies too large
		}
	}
}

void Arena::writeSegmentationStatistics(std::ostream& out) const
{
	const Attribute<MyBool>& isMissegmentedSS = frameAttributes.getFilled<MyBool>("isMissegmentedSS");
	const Attribute<MyBool>& isMissegmentedS = frameAttributes.getFilled<MyBool>("isMissegmentedS");
	const Attribute<MyBool>& isMissegmentedSL = frameAttributes.getFilled<MyBool>("isMissegmentedSL");
	const Attribute<MyBool>& isMissegmentedL = frameAttributes.getFilled<MyBool>("isMissegmentedL");
	const Attribute<MyBool>& isMissegmentedLL = frameAttributes.getFilled<MyBool>("isMissegmentedLL");
	const Attribute<MyBool>& isOcclusionTouched = frameAttributes.getFilled<MyBool>("isOcclusionTouched");

	std::vector<const Attribute<float>*> bodyAreas;
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		bodyAreas.push_back(&flyAttributes[flyNumber].getFilled<float>("bodyAreaEccentricityCorrectedTracked"));
	}

	const char delimiter = '\t';

	out <<
		std::accumulate(isMissegmentedSS.begin(), isMissegmentedSS.end(), 0) << delimiter <<
		std::accumulate(isMissegmentedS.begin(), isMissegmentedS.end(), 0) << delimiter <<
		std::accumulate(isMissegmentedSL.begin(), isMissegmentedSL.end(), 0) << delimiter <<
		std::accumulate(isMissegmentedL.begin(), isMissegmentedL.end(), 0) << delimiter <<
		std::accumulate(isMissegmentedLL.begin(), isMissegmentedLL.end(), 0) << delimiter <<
		getFrameCount() - std::accumulate(isOcclusionTouched.begin(), isOcclusionTouched.end(), 0);	// the number of frames that could possibly be in one of the categories above

	double areaSum = 0;
	size_t areaCount = 0;
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		if (!isOcclusionTouched[frameNumber]) {
			for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
				areaSum += (*bodyAreas[flyNumber])[frameNumber];
			}
			++areaCount;
		}
	}

	double areaMean = 0;
	if (areaCount >= 1) {
		areaMean = areaSum / static_cast<double>(areaCount);
		out << delimiter << areaMean / (pixelPerMillimeter * pixelPerMillimeter);
	} else {
		out << delimiter;
	}
	
	if (areaCount >= 2) {
		double squaredDifferenceSum = 0;
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (!isOcclusionTouched[frameNumber]) {
				double area = 0;
				for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
					area += (*bodyAreas[flyNumber])[frameNumber];
				}
				squaredDifferenceSum += (area - areaMean) * (area - areaMean);
			}
		}
		out << delimiter << sqrt(squaredDifferenceSum / (areaCount - 1)) / (pixelPerMillimeter * pixelPerMillimeter);
	} else {
		out << delimiter;
	}
	
	out << '\n';
}

void Arena::calculateTScores(float tPosLogisticRegressionCoefficient, float tBocLogisticRegressionCoefficient)
{
	const Attribute<MyBool>& interpolated = frameAttributes.getFilled<MyBool>("interpolated");
	const Attribute<float>& tBocScore = frameAttributes.getFilled<float>("tBocScore");
	Attribute<float>& tBocProb = frameAttributes.getEmpty<float>("tBocProb");
	Attribute<float>& tBoc = frameAttributes.getEmpty<float>("tBoc");
	Attribute<float>& tPosScore = frameAttributes.getEmpty<float>("tPosScore");
	Attribute<float>& tPosProb = frameAttributes.getEmpty<float>("tPosProb");
	Attribute<float>& tPos = frameAttributes.getEmpty<float>("tPos");
	Attribute<float>& tMovScore = frameAttributes.getEmpty<float>("tMovScore");
	Attribute<float>& tMovProb = frameAttributes.getEmpty<float>("tMovProb");
	Attribute<float>& tMov = frameAttributes.getEmpty<float>("tMov");
	Attribute<float>& tCombined = frameAttributes.getEmpty<float>("tCombined");

	{
		if (occlusionMap.size() == 0) {
			goto DONE_CALCULATING_SCORES;
		}

		size_t occlusionNumber = 0;	//TODO: gcc might complain about us skipping it with goto
		if (occlusionMap.begin.front() == 0) {
			// special case where the video starts with an occlusion, so we don't know about t
			occlusionMap.appendScores(0, 0, 0, tPosLogisticRegressionCoefficient, tBocLogisticRegressionCoefficient);
			++occlusionNumber;

			if (occlusionMap.end.front() == getFrameCount()) {
				// very special case where the video is occluded entirely
				goto DONE_CALCULATING_SCORES;
			}
		} else {
			// video doesn't start with an occlusion, but we add a 0-occlusion anyway to satisfy the hofacker precondition
			occlusionMap.prepend(0, 0);
			occlusionMap.appendScores(0, 0, 0, tPosLogisticRegressionCoefficient, tBocLogisticRegressionCoefficient);
			++occlusionNumber;
		}

		if (getFlyCount() == 1) {
			goto DONE_CALCULATING_SCORES;
		} else if (getFlyCount() == 2) {	//TODO: make this work for an arbitrary number of flies
			const Attribute<Vf2>& fly0Centroid = flyAttributes[0].getFilled<Vf2>("bodyCentroidTracked");
			const Attribute<Vf2>& fly1Centroid = flyAttributes[1].getFilled<Vf2>("bodyCentroidTracked");

			while (occlusionNumber != occlusionMap.size()) {
				if (occlusionMap.end[occlusionNumber] == getFrameCount()) {
					// video ends in this occlusion, so we don't know about t
					occlusionMap.appendScores(0, 0, 0, tPosLogisticRegressionCoefficient, tBocLogisticRegressionCoefficient);
					goto DONE_CALCULATING_SCORES;
				}

				// this is the common case of having an occlusion somewhere in the middle
				size_t beforeOcclusion = occlusionMap.begin[occlusionNumber] - 1;
				size_t afterOcclusion = occlusionMap.end[occlusionNumber];

				// Pos
				float firstToFirst = (fly0Centroid[beforeOcclusion] - fly0Centroid[afterOcclusion]).norm();
				float firstToSecond = (fly0Centroid[beforeOcclusion] - fly1Centroid[afterOcclusion]).norm();
				float secondToFirst = (fly1Centroid[beforeOcclusion] - fly0Centroid[afterOcclusion]).norm();
				float secondToSecond = (fly1Centroid[beforeOcclusion] - fly1Centroid[afterOcclusion]).norm();
				float straightDistance = firstToFirst + secondToSecond;
				float swappedDistance = firstToSecond + secondToFirst;
				float thisPosScore = (swappedDistance - straightDistance) / (straightDistance + swappedDistance);

				// Mov
				float thisMovScore = 0;
				if (beforeOcclusion >= 2 && !interpolated[beforeOcclusion - 2] && !interpolated[beforeOcclusion - 1] && !interpolated[beforeOcclusion] &&
					afterOcclusion + 2 < interpolated.size() && !interpolated[afterOcclusion] && !interpolated[afterOcclusion + 1] && !interpolated[afterOcclusion + 2]) {
					Vf2 fly0MovedBefore = fly0Centroid[beforeOcclusion] - fly0Centroid[beforeOcclusion - 2];
					Vf2 fly0MovedAfter = fly0Centroid[afterOcclusion + 2] - fly0Centroid[afterOcclusion];
					Vf2 fly1MovedBefore = fly1Centroid[beforeOcclusion] - fly1Centroid[beforeOcclusion - 2];
					Vf2 fly1MovedAfter = fly1Centroid[afterOcclusion + 2] - fly1Centroid[afterOcclusion];
					float firstToFirst = (fly0MovedBefore - fly0MovedAfter).norm();
					float firstToSecond = (fly0MovedBefore - fly1MovedAfter).norm();
					float secondToFirst = (fly1MovedBefore - fly0MovedAfter).norm();
					float secondToSecond = (fly1MovedBefore - fly1MovedAfter).norm();
					float straightDistance = firstToFirst + secondToSecond;
					float swappedDistance = firstToSecond + secondToFirst;
					thisMovScore = (swappedDistance - straightDistance) / (straightDistance + swappedDistance);
				}
				
				occlusionMap.appendScores(thisPosScore, thisMovScore, tBocScore[occlusionMap.begin[occlusionNumber]], tPosLogisticRegressionCoefficient, tBocLogisticRegressionCoefficient);
				++occlusionNumber;
			}
			// video doesn't end in an occlusion, so we add a 0 to satisfy the hofacker precondition
			occlusionMap.append(getFrameCount(), getFrameCount());
			occlusionMap.appendScores(0, 0, 0, tPosLogisticRegressionCoefficient, tBocLogisticRegressionCoefficient);
		}
	}

DONE_CALCULATING_SCORES:
	// write t values into frameAttributes for the entire occlusion
	tPosScore = std::vector<float>(getFrameCount(), 0);
	tPosProb = std::vector<float>(getFrameCount(), 0.5);
	tPos = std::vector<float>(getFrameCount(), 0);
	tMovScore = std::vector<float>(getFrameCount(), 0);
	tMovProb = std::vector<float>(getFrameCount(), 0.5);
	tMov = std::vector<float>(getFrameCount(), 0);
	tBocProb = std::vector<float>(getFrameCount(), 0.5);
	tBoc = std::vector<float>(getFrameCount(), 0);
	tCombined = std::vector<float>(getFrameCount(), 0);
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		for (size_t frameNumber = occlusionMap.begin[occlusionNumber]; frameNumber != occlusionMap.end[occlusionNumber]; ++frameNumber) {
			tPosScore[frameNumber] = occlusionMap.tPosScore[occlusionNumber];
			tPosProb[frameNumber] = occlusionMap.tPosProb[occlusionNumber];
			tPos[frameNumber] = occlusionMap.tPos[occlusionNumber];
			tMovScore[frameNumber] = occlusionMap.tMovScore[occlusionNumber];
			tMovProb[frameNumber] = occlusionMap.tMovProb[occlusionNumber];
			tMov[frameNumber] = occlusionMap.tMov[occlusionNumber];
			tBocProb[frameNumber] = occlusionMap.tBocProb[occlusionNumber];
			tBoc[frameNumber] = occlusionMap.tBoc[occlusionNumber];
			tCombined[frameNumber] = occlusionMap.tCombined[occlusionNumber];
		}
	}
}

void Arena::solveOcclusions(float sSizeWeight, bool discardMissegmentations)
{
	Attribute<MyBool>& occlusionInspected = frameAttributes.getEmpty<MyBool>("occlusionInspected");
	Attribute<uint32_t>& idPermutation = frameAttributes.getEmpty<uint32_t>("idPermutation");
	Attribute<float>& sSizeProb = frameAttributes.getEmpty<float>("sSizeProb");
	Attribute<float>& sSize = frameAttributes.getEmpty<float>("sSize");
	Attribute<float>& sCombined = frameAttributes.getEmpty<float>("sCombined");

	// this will be overridden in addAnnotation() if there is an annotation
	occlusionInspected.resize(getFrameCount());

	if (getFlyCount() != 1 && getFlyCount() != 2) {
		throw std::logic_error("cannot solve occlusions if flyCount is not 1 or 2");
	}

	if (occlusionMap.size() == 0 || getFlyCount() == 1) {
		//TODO: move these initializations out of the if ... this is a source of bugs
		idPermutation = std::vector<uint32_t>(getFrameCount(), 0);
		sSizeProb = std::vector<float>(getFrameCount(), 0.5);
		sSize = std::vector<float>(getFrameCount(), 0);
		sCombined = std::vector<float>(getFrameCount(), 0);
	} else if (getFlyCount() == 2) {
		//TODO: make this work for an arbitrary number of flies
		const Attribute<float>& fly0BodyArea = flyAttributes[0].getFilled<float>("bodyAreaEccentricityCorrectedTracked");
		const Attribute<float>& fly1BodyArea = flyAttributes[1].getFilled<float>("bodyAreaEccentricityCorrectedTracked");
		const Attribute<MyBool>& isMissegmented = frameAttributes.getFilled<MyBool>("isMissegmented");

		SequenceMap sequenceMap(occlusionMap, fly0BodyArea.getData(), fly1BodyArea.getData(), isMissegmented.getData(), sSizeWeight, discardMissegmentations);

		// short intermezzo: write s values into frameAttributes for the entire sequence
		sSizeProb = std::vector<float>(getFrameCount(), 0.5);
		sSize = std::vector<float>(getFrameCount(), 0);
		sCombined = std::vector<float>(getFrameCount(), 0);
		for (size_t sequenceNumber = 0; sequenceNumber != sequenceMap.size(); ++sequenceNumber) {
			for (size_t frameNumber = sequenceMap.begin[sequenceNumber]; frameNumber != sequenceMap.end[sequenceNumber]; ++frameNumber) {
				sSizeProb[frameNumber] = sequenceMap.sSizeProb[sequenceNumber];
				sSize[frameNumber] = sequenceMap.sSize[sequenceNumber];
				sCombined[frameNumber] = sequenceMap.sCombined[sequenceNumber];
			}
		}

		std::vector<bool> identityFlip = hofacker(sequenceMap.sCombined, occlusionMap.tCombined);
		bool firstIsFirst = true;
		bool beginsWithOcclusion = (occlusionMap.begin.front() == 0);
		size_t occlusionNumber = 0;
		if (!beginsWithOcclusion) {
			for (size_t frameNumber = sequenceMap.begin.front(); frameNumber != sequenceMap.end.front(); ++frameNumber) {
				idPermutation.push_back(1);
			}
		}
		for (; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			// add idPermutation indexes for the occlusion
			if (identityFlip[beginsWithOcclusion ? occlusionNumber : (occlusionNumber + 1)]) {
				firstIsFirst = !firstIsFirst;
				for (size_t frameNumber = occlusionMap.begin[occlusionNumber]; frameNumber != occlusionMap.end[occlusionNumber]; ++frameNumber) {
					idPermutation.push_back(1);
				}
			} else {
				for (size_t frameNumber = occlusionMap.begin[occlusionNumber]; frameNumber != occlusionMap.end[occlusionNumber]; ++frameNumber) {
					idPermutation.push_back(0);
				}
			}
			// add idPermutation indexes for the unoccluded sequence after the occlusion
			if (firstIsFirst) {
				for (size_t frameNumber = occlusionMap.end[occlusionNumber]; frameNumber != ((occlusionNumber + 1 == occlusionMap.size()) ? getFrameCount() : occlusionMap.begin[occlusionNumber + 1]); ++frameNumber) {
					idPermutation.push_back(0);
				}
			} else {
				for (size_t frameNumber = occlusionMap.end[occlusionNumber]; frameNumber != ((occlusionNumber + 1 == occlusionMap.size()) ? getFrameCount() : occlusionMap.begin[occlusionNumber + 1]); ++frameNumber) {
					idPermutation.push_back(1);
				}
			}
		}

		// rearrange all fly attributes "att" computed so far so that flyAttributes[i].att is the attribute vector "att" for fly i
		std::vector<bool> swapMask(idPermutation.size());
		std::copy(idPermutation.begin(), idPermutation.end(), swapMask.begin());
		frameAttributes.swap(swapMask);

		// fly identities shall be extended into the occlusion so the split bodies line up with the bodies in the frame before the occlusion
		// in other words, swapMask's semantics change here: "1" in an occluded frame no longer means that the occlusion has been flipped
		for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			if (occlusionMap.begin[occlusionNumber] != 0) {
				for (size_t frameNumber = occlusionMap.begin[occlusionNumber]; frameNumber != occlusionMap.end[occlusionNumber]; ++frameNumber) {
					swapMask[frameNumber] = swapMask[occlusionMap.begin[occlusionNumber] - 1];
				}
			}
		}
		flyAttributes[0].swap(flyAttributes[1], swapMask);

		//TODO: what about pairAttributes?! FOR NOW they're all derived and calculated later, so it's OK!
	}

/*	// sort flies by size - TODO: shouldn't be necessary unless there are NO occlusions or the sSize weight is 0
	//TODO: fly counts > 2
	if (getFlyCount() == 2) {
		//TODO: make this work for an arbitrary number of flies
		const Attribute<float>& fly0BodyArea = flyAttributes[0].getFilled<float>("bodyAreaEccentricityCorrected");
		const Attribute<float>& fly1BodyArea = flyAttributes[1].getFilled<float>("bodyAreaEccentricityCorrected");

		double fly0Size = std::accumulate(fly0BodyArea.begin(), fly0BodyArea.end(), 0.0);
		double fly1Size = std::accumulate(fly1BodyArea.begin(), fly1BodyArea.end(), 0.0);
		if (fly0Size > fly1Size) {
			frameAttributes.swapS(std::vector<bool>(getFrameCount(), true));	//TODO: swapS(): only swap s scores!!!
			std::swap(flyAttributes[0], flyAttributes[1]);
			//TODO: what about pairAttributes?! FOR NOW they're all derived and calculated later, so it's OK!
			for (size_t frameNumber = 0; frameNumber != idPermutation.size(); ++frameNumber) {
				idPermutation[frameNumber] = 1 - idPermutation[frameNumber];
			}
		}
	}
*/
	//TODO: just assert that idPermutation is zero; it should be, since the frameAttributes.swap() above should take care of that
	// zero-out idPermutation; from here on it will track any permutations performed AFTER occlusions have been solved
	//idPermutation.getData() = std::vector<size_t>(idPermutation.size());
}

void Arena::addAnnotations(const std::string& fileName)
{
	std::ifstream in(fileName.c_str());
	if (!in) {
		std::cout << "no annotations found\n";
		return;
	}

	std::string table;
	in.seekg(0, std::ios::end);   
	table.reserve(in.tellg());
	in.seekg(0, std::ios::beg);
	table.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());	// extra parentheses are required: "most vexing parse"
	table = ::transpose(table);

	Attribute<MyBool> occlusionInspectedAnn;
	std::vector<Attribute<Vf2> > bodyCentroidsAnn(getFlyCount());

	std::istringstream attributeStream(table);
	const char delimiter = '\t';
	size_t lineNumber = 0;
	for (std::string line; std::getline(attributeStream, line); ++lineNumber) {
		std::vector<std::string> splitLine = ::split(line);
		if (splitLine.size() < 4) {
			std::cerr << "addAnnotations: column " << lineNumber << " is not valid; skipping...\n";
			continue;
		}
		if (splitLine[0] != "" && splitLine[1] == "bodyCentroid") {	// fly attribute or pair attribute
			std::vector<std::string> splitFlyNumber = ::split(splitLine[0], ' ');
			if (splitFlyNumber.size() == 1) {	// fly attribute
				size_t flyNumber = 0;
				try {
					flyNumber = unstringify<size_t>(splitFlyNumber[0]);
				} catch (std::runtime_error&) {
					std::cerr << "addAnnotations: unknown fly number in column " << lineNumber << "; skipping...\n";
					continue;
				}
				if (flyNumber >= getFlyCount()) {
					std::cerr << "addAnnotations: fly number out of range in column " << lineNumber << "; skipping...\n";
					continue;
				}
				if (bodyCentroidsAnn[flyNumber].empty()) {
					bodyCentroidsAnn[flyNumber].read(splitLine.begin() + 4, splitLine.end());
				} else {
					std::cerr << "addAnnotations: duplicate column " << lineNumber << " found; skipping...\n";
					continue;
				}
			}
		}
		if (splitLine[0] == "" && splitLine[1] == "occlusionInspected") {	// frame attribute
			if (occlusionInspectedAnn.empty()) {
				occlusionInspectedAnn.read(splitLine.begin() + 4, splitLine.end());
			} else {
				std::cerr << "addAnnotations: duplicate column " << lineNumber << " found; skipping...\n";
				continue;
			}
		}
	}

	// sanity check: make sure the attribute sizes are consistent
	if (occlusionInspectedAnn.size() != getFrameCount()) {
		throw std::runtime_error("annotation file data is incompatible with tracking data");
	}
	frameAttributes.getFilled<MyBool>("occlusionInspected").getData() = occlusionInspectedAnn.getData();

	// sanity check: make sure the attribute sizes are consistent
	for (std::vector<Attribute<Vf2> >::const_iterator iter = bodyCentroidsAnn.begin(); iter != bodyCentroidsAnn.end(); ++iter) {
		if (iter->size() != getFrameCount()) {
			throw std::runtime_error("annotation file data is incompatible with tracking data");
		}
	}

	if (getFlyCount() != 2) {	// this doesn't work for fly counts != 2 because of the pairwise swapping below
		throw std::logic_error("cannot add annotations if flyCount is not 2");
	}

	//TODO: this works for 2 flies only, until we have a function that gives us a pairwise swapping strategy to accomplish the beforeToNow mapping
	std::vector<bool> swapMask(getFrameCount());
	bool isSwapped = false;
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		if (occlusionMap.end[occlusionNumber] != getFrameCount()) {
			cv::Mat distanceMatrix(getFlyCount(), getFlyCount(), CV_32F);
			for (size_t flyBefore = 0; flyBefore != getFlyCount(); ++flyBefore) {
				for (size_t flyNow = 0; flyNow != getFlyCount(); ++flyNow) {
					const Attribute<Vf2>& bodyCentroid = flyAttributes[flyBefore].getFilled<Vf2>("bodyCentroidTracked");
					const Attribute<Vf2>& bodyCentroidAnn = bodyCentroidsAnn[flyNow];
					distanceMatrix.at<float>(flyBefore, flyNow) = (
						bodyCentroid[occlusionMap.end[occlusionNumber]] -
						bodyCentroidAnn[occlusionMap.end[occlusionNumber]]
					).norm();
				}
			}
			std::vector<size_t> beforeToNow = hungarian(distanceMatrix);

			// if occlusion[occlusionNumber] was resolved differently from the annotation, swap the data for this occlusion
			if ((!isSwapped && beforeToNow[0] == 1) || (isSwapped && beforeToNow[0] == 0)) {
				swapMask.clear();
				swapMask.resize(getFrameCount());
				for (size_t frameNumber = occlusionMap.begin[occlusionNumber]; frameNumber != occlusionMap.end[occlusionNumber]; ++frameNumber) {
					swapMask[frameNumber] = true;
				}
				frameAttributes.swap(swapMask);
				flyAttributes[0].swap(flyAttributes[1], swapMask);
				//TODO: what about pairAttributes?! FOR NOW they're all derived and calculated later, so it's OK!
				isSwapped = !isSwapped;
			}

			// if we're swapped, swap the data for the sequence that follows
			if (isSwapped) {
				swapMask.clear();
				swapMask.resize(getFrameCount());
				for (size_t frameNumber = occlusionMap.end[occlusionNumber]; frameNumber != occlusionMap.begin[occlusionNumber + 1]; ++frameNumber) {
					swapMask[frameNumber] = true;
				}
				frameAttributes.swap(swapMask);
				flyAttributes[0].swap(flyAttributes[1], swapMask);
				//TODO: what about pairAttributes?! FOR NOW they're all derived and calculated later, so it's OK!
			}
		}
	}
}

void Arena::writeOcclusionReport(std::ostream& out) const
{
	const char delimiter = '\t';
	std::ostringstream trans;

	trans << "Occlusion";
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		trans << delimiter << occlusionNumber;
	}
	trans << '\n';
	trans << "Begin";
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		trans << delimiter << occlusionMap.begin[occlusionNumber];
	}
	trans << '\n';
	trans << "End";
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		trans << delimiter << occlusionMap.end[occlusionNumber];
	}
	trans << '\n';

	const Attribute<uint32_t>& idPermutation = frameAttributes.getFilled<uint32_t>("idPermutation");
	trans << "idPermutation before";
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		size_t begin = occlusionMap.begin[occlusionNumber];
		trans << delimiter;
		if (begin != 0) {
			trans << idPermutation[begin - 1];
		}
	}
	trans << '\n';
	trans << "idPermutation during";
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		size_t begin = occlusionMap.begin[occlusionNumber];
		trans << delimiter;
		if (begin != idPermutation.size()) {
			trans << idPermutation[begin];
		}
	}
	trans << '\n';
	trans << "idPermutation after";
	for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
		size_t end = occlusionMap.end[occlusionNumber];
		trans << delimiter;
		if (end != idPermutation.size()) {
			trans << idPermutation[end];
		}
	}
	trans << '\n';

	std::vector<std::string> sScores = frameAttributes.getOcclusionSScoreNames();
	std::vector<std::string> sProbs = frameAttributes.getOcclusionSProbNames();
	std::vector<std::string> tScores = frameAttributes.getOcclusionTScoreNames();
	std::vector<std::string> tProbs = frameAttributes.getOcclusionTProbNames();

	for (std::vector<std::string>::const_iterator iter = sScores.begin(); iter != sScores.end(); ++iter) {
		const Attribute<float>& attribute = frameAttributes.getFilled<float>(*iter);
		trans << attribute.getName() << " before";
		for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			size_t begin = occlusionMap.begin[occlusionNumber];
			trans << delimiter;
			if (begin != 0) {
				trans << attribute[begin - 1];
			}
		}
		trans << '\n';
	}
	for (std::vector<std::string>::const_iterator iter = sProbs.begin(); iter != sProbs.end(); ++iter) {
		const Attribute<float>& attribute = frameAttributes.getFilled<float>(*iter);
		trans << attribute.getName() << " before";
		for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			size_t begin = occlusionMap.begin[occlusionNumber];
			trans << delimiter;
			if (begin != 0) {
				trans << attribute[begin - 1];
			}
		}
		trans << '\n';
	}

	for (std::vector<std::string>::const_iterator iter = tScores.begin(); iter != tScores.end(); ++iter) {
		const Attribute<float>& attribute = frameAttributes.getFilled<float>(*iter);
		trans << attribute.getName();
		for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			size_t begin = occlusionMap.begin[occlusionNumber];
			trans << delimiter;
			if (begin != attribute.size()) {
				trans << attribute[begin];
			}
		}
		trans << '\n';
	}
	for (std::vector<std::string>::const_iterator iter = tProbs.begin(); iter != tProbs.end(); ++iter) {
		const Attribute<float>& attribute = frameAttributes.getFilled<float>(*iter);
		trans << attribute.getName();
		for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			size_t begin = occlusionMap.begin[occlusionNumber];
			trans << delimiter;
			if (begin != attribute.size()) {
				trans << attribute[begin];
			}
		}
		trans << '\n';
	}

	for (std::vector<std::string>::const_iterator iter = sScores.begin(); iter != sScores.end(); ++iter) {
		const Attribute<float>& attribute = frameAttributes.getFilled<float>(*iter);
		trans << attribute.getName() << " after";
		for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			size_t end = occlusionMap.end[occlusionNumber];
			trans << delimiter;
			if (end != attribute.size()) {
				trans << attribute[end];
			}
		}
		trans << '\n';
	}
	for (std::vector<std::string>::const_iterator iter = sProbs.begin(); iter != sProbs.end(); ++iter) {
		const Attribute<float>& attribute = frameAttributes.getFilled<float>(*iter);
		trans << attribute.getName() << " after";
		for (size_t occlusionNumber = 0; occlusionNumber != occlusionMap.size(); ++occlusionNumber) {
			size_t end = occlusionMap.end[occlusionNumber];
			trans << delimiter;
			if (end != attribute.size()) {
				trans << attribute[end];
			}
		}
		trans << '\n';
	}

	out << ::transpose(trans.str());
}

void Arena::interpolateAttributes()
{
	const Attribute<MyBool>& interpolated = frameAttributes.getFilled<MyBool>("interpolated");
	const Attribute<MyBool>& interpolatedAbsolutely = frameAttributes.getFilled<MyBool>("interpolatedAbsolutely");
	const Attribute<MyBool>& interpolatedRelatively = frameAttributes.getFilled<MyBool>("interpolatedRelatively");

	/*TODO: const */Attribute<Vf2>& averageBodyCentroid = frameAttributes.getFilled<Vf2>("averageBodyCentroid");
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& bodyCentroidTracked = flyAttributes[flyNumber].getFilled<Vf2>("bodyCentroidTracked");
		const Attribute<float>& bodyAreaTracked = flyAttributes[flyNumber].getFilled<float>("bodyAreaTracked");
		const Attribute<float>& bodyAreaEccentricityCorrectedTracked = flyAttributes[flyNumber].getFilled<float>("bodyAreaEccentricityCorrectedTracked");
		const Attribute<float>& bodyMajorAxisLengthTracked = flyAttributes[flyNumber].getFilled<float>("bodyMajorAxisLengthTracked");
		const Attribute<float>& bodyMinorAxisLengthTracked = flyAttributes[flyNumber].getFilled<float>("bodyMinorAxisLengthTracked");

		Attribute<Vf2>& bodyCentroid = flyAttributes[flyNumber].getEmpty<Vf2>("bodyCentroid");
		Attribute<float>& bodyArea = flyAttributes[flyNumber].getEmpty<float>("bodyArea");
		Attribute<float>& bodyAreaEccentricityCorrected = flyAttributes[flyNumber].getEmpty<float>("bodyAreaEccentricityCorrected");
		Attribute<float>& bodyMajorAxisLength = flyAttributes[flyNumber].getEmpty<float>("bodyMajorAxisLength");
		Attribute<float>& bodyMinorAxisLength = flyAttributes[flyNumber].getEmpty<float>("bodyMinorAxisLength");

		bodyCentroid.getData() = bodyCentroidTracked.getData();
		bodyArea.getData() = bodyAreaTracked.getData();
		bodyAreaEccentricityCorrected.getData() = bodyAreaEccentricityCorrectedTracked.getData();
		bodyMajorAxisLength.getData() = bodyMajorAxisLengthTracked.getData();
		bodyMinorAxisLength.getData() = bodyMinorAxisLengthTracked.getData();

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (interpolated[frameNumber]) {
				assert(frameNumber != 0);	// there must be a frame before...we made sure in prepareInterpolation()!
				size_t frameBefore = frameNumber - 1;
				size_t frameAfter = 0;
				for (size_t frameDelta = 1; ;++frameDelta) {
					assert(frameNumber + frameDelta != getFrameCount());	// there must be a frame after...we made sure in prepareInterpolation()!
					if (!interpolated[frameNumber + frameDelta]) {
						frameAfter = frameNumber + frameDelta;
						break;
					}
				}
				interpolate(bodyMajorAxisLength.begin() + frameBefore, bodyMajorAxisLength.begin() + frameAfter);
				interpolate(bodyMinorAxisLength.begin() + frameBefore, bodyMinorAxisLength.begin() + frameAfter);
				interpolate(bodyArea.begin() + frameBefore, bodyArea.begin() + frameAfter);
				interpolate(bodyAreaEccentricityCorrected.begin() + frameBefore, bodyAreaEccentricityCorrected.begin() + frameAfter);

				frameNumber = frameAfter;
			}
		}

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (interpolatedAbsolutely[frameNumber]) {
				assert(frameNumber != 0);	// there must be a frame before...we made sure in prepareInterpolation()!
				size_t frameBefore = frameNumber - 1;
				size_t frameAfter = 0;
				for (size_t frameDelta = 1; ;++frameDelta) {
					assert(frameNumber + frameDelta != getFrameCount());	// there must be a frame after...we made sure in prepareInterpolation()!
					if (!interpolatedAbsolutely[frameNumber + frameDelta]) {
						frameAfter = frameNumber + frameDelta;
						break;
					}
				}
				interpolate(bodyCentroid.begin() + frameBefore, bodyCentroid.begin() + frameAfter);

				frameNumber = frameAfter;
			}
		}

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (interpolatedRelatively[frameNumber]) {
				assert(frameNumber != 0);	// there must be a frame before...we made sure in prepareInterpolation()!
				size_t frameBefore = frameNumber - 1;
				size_t frameAfter = 0;
				for (size_t frameDelta = 1; ;++frameDelta) {
					assert(frameNumber + frameDelta != getFrameCount());	// there must be a frame after...we made sure in prepareInterpolation()!
					if (!interpolatedRelatively[frameNumber + frameDelta]) {
						frameAfter = frameNumber + frameDelta;
						break;
					}
				}
				interpolateRelative(bodyCentroid.begin() + frameBefore, bodyCentroid.begin() + frameAfter, averageBodyCentroid.begin() + frameBefore);

				frameNumber = frameAfter;
			}
		}

		// zero out headingFromWings for interpolated frames
		const Attribute<float>& headingFromWingsTracked = flyAttributes[flyNumber].getFilled<float>("headingFromWingsTracked");
		Attribute<float>& headingFromWings = flyAttributes[flyNumber].getEmpty<float>("headingFromWings");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			headingFromWings.push_back(interpolated[frameNumber] ? 0 : headingFromWingsTracked[frameNumber]);
		}
	}
}

void Arena::deriveHeadingIndependentAttributes()
{
	// distance from arena center
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& bodyCentroid = flyAttributes[flyNumber].getFilled<Vf2>("bodyCentroid");
		Attribute<float>& distanceFromArenaCenter = flyAttributes[flyNumber].getEmpty<float>("distanceFromArenaCenter");
		Vf2 arenaCenter = makeVec(getBoundingBox().width / 2.0f, getBoundingBox().height / 2.0f);
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			distanceFromArenaCenter.push_back((bodyCentroid[frameNumber] - arenaCenter).norm());
		}
	}

	// eccentricity from major and minor axis
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyMajorAxisLength = flyAttributes[flyNumber].getFilled<float>("bodyMajorAxisLength");
		const Attribute<float>& bodyMinorAxisLength = flyAttributes[flyNumber].getFilled<float>("bodyMinorAxisLength");
		Attribute<float>& bodyEccentricity = flyAttributes[flyNumber].getEmpty<float>("bodyEccentricity");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			bodyEccentricity.push_back(eccentricity(bodyMajorAxisLength[frameNumber], bodyMinorAxisLength[frameNumber]));
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& wingMajorAxisLength = flyAttributes[flyNumber].getFilled<float>("wingMajorAxisLength");
		const Attribute<float>& wingMinorAxisLength = flyAttributes[flyNumber].getFilled<float>("wingMinorAxisLength");
		Attribute<float>& wingEccentricity = flyAttributes[flyNumber].getEmpty<float>("wingEccentricity");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			wingEccentricity.push_back(eccentricity(wingMajorAxisLength[frameNumber], wingMinorAxisLength[frameNumber]));
		}
	}

	// gauss filter x and y
	int filterWidth = 25;	// should be odd
	std::vector<float> gaussKernel = discreteGaussian<float>(filterWidth);
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& bodyCentroid = flyAttributes[flyNumber].getFilled<Vf2>("bodyCentroid");
		Attribute<Vf2>& filteredBodyCentroid = flyAttributes[flyNumber].getEmpty<Vf2>("filteredBodyCentroid");
		filteredBodyCentroid.getData() = convolve_clamp(bodyCentroid.getData(), gaussKernel);
	}

	// calculate distance moved and the global direction moved
	std::vector<float> deriveKernel;
	deriveKernel.push_back(-1);
	deriveKernel.push_back(1);
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& filteredBodyCentroid = flyAttributes[flyNumber].getFilled<Vf2>("filteredBodyCentroid");
		Attribute<Vf2>& moved = flyAttributes[flyNumber].getEmpty<Vf2>("moved");
		moved.getData() = convolve_clamp(filteredBodyCentroid.getData(), deriveKernel);

		Attribute<float>& movedAbs = flyAttributes[flyNumber].getEmpty<float>("movedAbs");
		Attribute<float>& movedDirectionGlobal = flyAttributes[flyNumber].getEmpty<float>("movedDirectionGlobal");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			movedAbs.push_back(moved[frameNumber].norm());
			
			// direction moved in the global coordinate system
			float movedX = moved[frameNumber].x();
			float movedY = moved[frameNumber].y();
			float direction = std::atan2(movedY, movedX) * 180.0 / CV_PI;
			if (direction < 0) {
				direction += 360;
			}
			movedDirectionGlobal.push_back(direction);
		}
	}
	
	// calculate distance to other fly
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<Vf2>& active_bodyCentroid = flyAttributes[activeFly].getFilled<Vf2>("bodyCentroid");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<Vf2>& passive_bodyCentroid = flyAttributes[passiveFly].getFilled<Vf2>("bodyCentroid");
			Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getEmpty<float>("distanceBodyBody");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				distanceBodyBody.push_back((passive_bodyCentroid[frameNumber] - active_bodyCentroid[frameNumber]).norm());
			}
		}
	}
}

void Arena::solveHeading(float sMotionWeight, float sWingsWeight, float sMaxMotionWingsWeight, float sColorWeight, float tBeforeWeight)
{
	std::vector<MyBool> notInterpolated = !(frameAttributes.getFilled<MyBool>("interpolated").getData());

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		// calculate heading based on persistence of orientation
		const std::vector<Vf2> bodyCentroid = logicalIndex(flyAttributes[flyNumber].getFilled<Vf2>("bodyCentroid").getData(), notInterpolated);
		const std::vector<float> bodyMajorAxisLength = logicalIndex(flyAttributes[flyNumber].getFilled<float>("bodyMajorAxisLength").getData(), notInterpolated);
		const std::vector<float> bodyOrientationTracked = logicalIndex(flyAttributes[flyNumber].getFilled<float>("bodyOrientationTracked").getData(), notInterpolated);
		const std::vector<float> bodyEccentricity = logicalIndex(flyAttributes[flyNumber].getFilled<float>("bodyEccentricity").getData(), notInterpolated);
		std::vector<float> headingFromBefore;

		headingFromBefore.push_back(0);
		for (int frameNumber = 1; frameNumber < bodyCentroid.size(); ++frameNumber) {
			Vf2 leftTipBefore(bodyCentroid[frameNumber - 1]);
			Vf2 rightTipBefore(bodyCentroid[frameNumber - 1]);
			{
				double radAngle = bodyOrientationTracked[frameNumber - 1] * CV_PI / 180.;
				Vf2 cs = makeVec((float)cos(radAngle), (float)sin(radAngle)) * 0.5f;
				leftTipBefore -= cs * bodyMajorAxisLength[frameNumber - 1];
				rightTipBefore += cs * bodyMajorAxisLength[frameNumber - 1];
			}
			
			Vf2 leftTipNow(bodyCentroid[frameNumber]);
			Vf2 rightTipNow(bodyCentroid[frameNumber]);
			{
				double radAngle = bodyOrientationTracked[frameNumber] * CV_PI / 180.;
				Vf2 cs = makeVec((float)cos(radAngle), (float)sin(radAngle)) * 0.5f;
				leftTipNow -= cs * bodyMajorAxisLength[frameNumber];
				rightTipNow += cs * bodyMajorAxisLength[frameNumber];
			}
			
			float distLL = (leftTipBefore - leftTipNow).norm();
			float distRR = (rightTipBefore - rightTipNow).norm();
			float distLR = (leftTipBefore - rightTipNow).norm();
			float distRL = (rightTipBefore - leftTipNow).norm();

			// yields negative values if the heading changed (head was left and is now right or vice versa); positive ones if it didn't
			float score = (distLR + distRL - distLL - distRR) / (distLR + distRL + distLL + distRR);
			score *= bodyEccentricity[frameNumber];
			headingFromBefore.push_back(prob2logodd(score2prob(score)));
		}

		flyAttributes[flyNumber].getEmpty<float>("headingFromBefore").getData() = spread(headingFromBefore, notInterpolated);

		// calculate heading based on motion
		std::vector<float> headingFromMotion;

		headingFromMotion.push_back(0);
		headingFromMotion.push_back(0);
		for (int frameNumber = 2; frameNumber < bodyCentroid.size(); ++frameNumber) {
			Vf2 centroidBefore(bodyCentroid[frameNumber - 2]);
			
			Vf2 leftTipNow(bodyCentroid[frameNumber]);
			Vf2 rightTipNow(bodyCentroid[frameNumber]);
			{
				double radAngle = bodyOrientationTracked[frameNumber] * CV_PI / 180.;
				Vf2 cs = makeVec((float)cos(radAngle), (float)sin(radAngle)) * 0.5f;
				leftTipNow -= cs * bodyMajorAxisLength[frameNumber];
				rightTipNow += cs * bodyMajorAxisLength[frameNumber];
			}
			
			float distLC2 = (leftTipNow - centroidBefore).norm();
			float distRC2 = (rightTipNow - centroidBefore).norm();

			//TODO: check the sign and direction
			float score = -(distLC2 - distRC2) / (distLC2 + distRC2);
			headingFromMotion.push_back(prob2logodd(score2prob(score)));
		}

		flyAttributes[flyNumber].getEmpty<float>("headingFromMotion").getData() = spread(headingFromMotion, notInterpolated);

		// calculate heading based on max(motion, wings)
		const std::vector<float> headingFromWings = logicalIndex(flyAttributes[flyNumber].getFilled<float>("headingFromWings").getData(), notInterpolated);
		std::vector<float> headingFromMaxMotionWings;

		for (int frameNumber = 0; frameNumber < headingFromWings.size(); ++frameNumber) {
			float motionAbs = std::abs(headingFromMotion[frameNumber]);
			float wingsAbs = std::abs(headingFromWings[frameNumber]);
			if (motionAbs > wingsAbs) {
				headingFromMaxMotionWings.push_back(headingFromMotion[frameNumber]);
			} else {
				headingFromMaxMotionWings.push_back(headingFromWings[frameNumber]);
			}
		}

		flyAttributes[flyNumber].getEmpty<float>("headingFromMaxMotionWings").getData() = spread(headingFromMaxMotionWings, notInterpolated);

		// combine s and combine t
		const std::vector<float> headingFromColor = logicalIndex(flyAttributes[flyNumber].getFilled<float>("headingFromColor").getData(), notInterpolated);
		std::vector<float> headingSCombined;
		std::vector<float> headingTCombined;

		for (int frameNumber = 0; frameNumber < headingFromWings.size(); ++frameNumber) {
			headingSCombined.push_back(
				headingFromMotion[frameNumber] * sMotionWeight +
				headingFromWings[frameNumber] * sWingsWeight +
				headingFromMaxMotionWings[frameNumber] * sMaxMotionWingsWeight +
				headingFromColor[frameNumber] * sColorWeight
			);
			headingTCombined.push_back(
				headingFromBefore[frameNumber] * tBeforeWeight
			);
		}

		flyAttributes[flyNumber].getEmpty<float>("headingSCombined").getData() = spread(headingSCombined, notInterpolated);
		flyAttributes[flyNumber].getEmpty<float>("headingTCombined").getData() = spread(headingTCombined, notInterpolated);

		// run the hofacker
		std::vector<float> filteredHeading;

		std::vector<float> headingTCombinedWithTrailingZero = headingTCombined;
		headingTCombinedWithTrailingZero.push_back(0);
		std::vector<bool> headingFlip = hofacker(headingSCombined, headingTCombinedWithTrailingZero);
		bool facingLeft = false;
		for (int frameNumber = 0; frameNumber != headingFlip.size() - 1; ++frameNumber) {
			if (headingFlip[frameNumber]) {
				facingLeft = !facingLeft;
			}
			if (facingLeft) {
				filteredHeading.push_back(-1);
			} else {
				filteredHeading.push_back(1);
			}
		}

		flyAttributes[flyNumber].getEmpty<float>("filteredHeading").getData() = spread(filteredHeading, notInterpolated);
	}

	//TODO: only for moonwalker!
/*	// median filter heading over 1 second
	int headingFilterWidth = 25;	// should be odd
	std::vector<ptrdiff_t> neighborhood;
	for (ptrdiff_t offset = -(headingFilterWidth / 2); offset <= (headingFilterWidth / 2); ++offset) {
		neighborhood.push_back(offset);
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		flyAttributes[flyNumber].filteredHeading = ordfilt(flyAttributes[flyNumber].filteredHeading, 0.5, neighborhood);
	}
*/	
	// add 180° to the orientation for flies facing left (TODO: left means up)
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& filteredHeading = flyAttributes[flyNumber].getFilled<float>("filteredHeading");
		const Attribute<float>& bodyOrientationTracked = flyAttributes[flyNumber].getFilled<float>("bodyOrientationTracked");
		Attribute<float>& bodyOrientationFlipped = flyAttributes[flyNumber].getEmpty<float>("bodyOrientationFlipped");

		bodyOrientationFlipped.getData() = bodyOrientationTracked.getData();
		for (size_t frameNumber = 0; frameNumber != filteredHeading.size(); ++frameNumber) {
			if (filteredHeading[frameNumber] == -1) {
				bodyOrientationFlipped[frameNumber] = bodyOrientationTracked[frameNumber] + 180;
			}
		}
	}
}

void Arena::interpolateOrientation()
{
	const Attribute<MyBool>& interpolated = frameAttributes.getFilled<MyBool>("interpolated");

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyOrientationFlipped = flyAttributes[flyNumber].getFilled<float>("bodyOrientationFlipped");

		Attribute<float>& bodyOrientation = flyAttributes[flyNumber].getEmpty<float>("bodyOrientation");

		bodyOrientation.getData() = bodyOrientationFlipped.getData();

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (interpolated[frameNumber]) {
				assert(frameNumber != 0);	// there must be a frame before...we made sure in prepareInterpolation()!
				size_t frameBefore = frameNumber - 1;
				size_t frameAfter = 0;
				for (size_t frameDelta = 1; ;++frameDelta) {
					assert(frameNumber + frameDelta != getFrameCount());	// there must be a frame after...we made sure in prepareInterpolation()!
					if (!interpolated[frameNumber + frameDelta]) {
						frameAfter = frameNumber + frameDelta;
						break;
					}
				}
				interpolate_wrap(bodyOrientation.begin() + frameBefore, bodyOrientation.begin() + frameAfter, 360);

				frameNumber = frameAfter;
			}
		}

		// assign heading values for interpolated frames
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			//TODO: we're CHANGING filteredHeading here, violating our single assignment rule!
			Attribute<float>& filteredHeading = flyAttributes[flyNumber].getFilled<float>("filteredHeading");
			if (interpolated[frameNumber]) {
				if (bodyOrientation[frameNumber] < 180) {
					filteredHeading[frameNumber] = 1;
				} else {
					filteredHeading[frameNumber] = -1;
				}
			}
		}
	}
}

void Arena::selectQuadrants()
{
	// pick the wing attributes from the appropriate quadrants
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& bottomRightWingTip = flyAttributes[flyNumber].getFilled<Vf2>("bottomRightWingTip");
		const Attribute<Vf2>& bottomLeftWingTip = flyAttributes[flyNumber].getFilled<Vf2>("bottomLeftWingTip");
		const Attribute<Vf2>& topLeftWingTip = flyAttributes[flyNumber].getFilled<Vf2>("topLeftWingTip");
		const Attribute<Vf2>& topRightWingTip = flyAttributes[flyNumber].getFilled<Vf2>("topRightWingTip");

		const Attribute<float>& bottomRightBodyArea = flyAttributes[flyNumber].getFilled<float>("bottomRightBodyArea");
		const Attribute<float>& bottomLeftBodyArea = flyAttributes[flyNumber].getFilled<float>("bottomLeftBodyArea");
		const Attribute<float>& topLeftBodyArea = flyAttributes[flyNumber].getFilled<float>("topLeftBodyArea");
		const Attribute<float>& topRightBodyArea = flyAttributes[flyNumber].getFilled<float>("topRightBodyArea");

		const Attribute<float>& bottomRightWingArea = flyAttributes[flyNumber].getFilled<float>("bottomRightWingArea");
		const Attribute<float>& bottomLeftWingArea = flyAttributes[flyNumber].getFilled<float>("bottomLeftWingArea");
		const Attribute<float>& topLeftWingArea = flyAttributes[flyNumber].getFilled<float>("topLeftWingArea");
		const Attribute<float>& topRightWingArea = flyAttributes[flyNumber].getFilled<float>("topRightWingArea");

		const Attribute<float>& bottomRightWingAngle = flyAttributes[flyNumber].getFilled<float>("bottomRightWingAngle");
		const Attribute<float>& bottomLeftWingAngle = flyAttributes[flyNumber].getFilled<float>("bottomLeftWingAngle");
		const Attribute<float>& topLeftWingAngle = flyAttributes[flyNumber].getFilled<float>("topLeftWingAngle");
		const Attribute<float>& topRightWingAngle = flyAttributes[flyNumber].getFilled<float>("topRightWingAngle");

		const Attribute<float>& filteredHeading = flyAttributes[flyNumber].getFilled<float>("filteredHeading");

		Attribute<Vf2>& leftWingTip = flyAttributes[flyNumber].getEmpty<Vf2>("leftWingTip");
		Attribute<Vf2>& rightWingTip = flyAttributes[flyNumber].getEmpty<Vf2>("rightWingTip");

		Attribute<float>& leftBodyArea = flyAttributes[flyNumber].getEmpty<float>("leftBodyArea");
		Attribute<float>& rightBodyArea = flyAttributes[flyNumber].getEmpty<float>("rightBodyArea");

		Attribute<float>& leftWingArea = flyAttributes[flyNumber].getEmpty<float>("leftWingArea");
		Attribute<float>& rightWingArea = flyAttributes[flyNumber].getEmpty<float>("rightWingArea");

		Attribute<float>& leftWingAngle = flyAttributes[flyNumber].getEmpty<float>("leftWingAngle");
		Attribute<float>& rightWingAngle = flyAttributes[flyNumber].getEmpty<float>("rightWingAngle");

		for (size_t frameNumber = 0; frameNumber != filteredHeading.size(); ++frameNumber) {
			if (filteredHeading[frameNumber] == -1) {	// facing up
				leftWingTip.push_back(bottomLeftWingTip[frameNumber]);
				leftBodyArea.push_back(bottomLeftBodyArea[frameNumber]);
				leftWingArea.push_back(bottomLeftWingArea[frameNumber]);
				leftWingAngle.push_back(bottomLeftWingAngle[frameNumber]);
				rightWingTip.push_back(bottomRightWingTip[frameNumber]);
				rightBodyArea.push_back(bottomRightBodyArea[frameNumber]);
				rightWingArea.push_back(bottomRightWingArea[frameNumber]);
				rightWingAngle.push_back(bottomRightWingAngle[frameNumber]);
			} else {	// facing down
				leftWingTip.push_back(topRightWingTip[frameNumber]);
				leftBodyArea.push_back(topRightBodyArea[frameNumber]);
				leftWingArea.push_back(topRightWingArea[frameNumber]);
				leftWingAngle.push_back(topRightWingAngle[frameNumber]);
				rightWingTip.push_back(topLeftWingTip[frameNumber]);
				rightBodyArea.push_back(topLeftBodyArea[frameNumber]);
				rightWingArea.push_back(topLeftWingArea[frameNumber]);
				rightWingAngle.push_back(topLeftWingAngle[frameNumber]);
			}
		}
	}
}

void Arena::deriveHeadingDependentAttributes()
{
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyOrientation = flyAttributes[flyNumber].getFilled<float>("bodyOrientation");
		const Attribute<float>& movedDirectionGlobal = flyAttributes[flyNumber].getFilled<float>("movedDirectionGlobal");
		Attribute<float>& movedDirectionLocal = flyAttributes[flyNumber].getEmpty<float>("movedDirectionLocal");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			movedDirectionLocal.push_back(angleDifference(bodyOrientation[frameNumber], movedDirectionGlobal[frameNumber]));
		}
	}

	// turning
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyOrientation = flyAttributes[flyNumber].getFilled<float>("bodyOrientation");
		const Attribute<Vf2>& filteredBodyCentroid = flyAttributes[flyNumber].getFilled<Vf2>("filteredBodyCentroid");
		Attribute<float>& turned = flyAttributes[flyNumber].getEmpty<float>("turned");
		Attribute<float>& turnedAbs = flyAttributes[flyNumber].getEmpty<float>("turnedAbs");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			if (frameNumber > 0) {
				turned.push_back(angleDifference(bodyOrientation[frameNumber - 1], bodyOrientation[frameNumber]));
				turnedAbs.push_back(std::abs(turned.back()));
			} else {
				turned.push_back(0);
				turnedAbs.push_back(0);
			}
		}
	}

	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<Vf2>& active_bodyCentroid = flyAttributes[activeFly].getFilled<Vf2>("bodyCentroid");
		const Attribute<float>& active_bodyOrientation = flyAttributes[activeFly].getFilled<float>("bodyOrientation");
		const Attribute<float>& active_bodyMajorAxisLength = flyAttributes[activeFly].getFilled<float>("bodyMajorAxisLength");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<Vf2>& passive_bodyCentroid = flyAttributes[passiveFly].getFilled<Vf2>("bodyCentroid");
			const Attribute<float>& passive_bodyOrientation = flyAttributes[passiveFly].getFilled<float>("bodyOrientation");
			const Attribute<float>& passive_bodyMajorAxisLength = flyAttributes[passiveFly].getFilled<float>("bodyMajorAxisLength");
			Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getEmpty<float>("angleToOther");
			Attribute<float>& angleSubtended = pairAttributes[activeFly][passiveFly].getEmpty<float>("angleSubtended");
			Attribute<float>& distanceHeadBody = pairAttributes[activeFly][passiveFly].getEmpty<float>("distanceHeadBody");
			Attribute<float>& distanceHeadTail = pairAttributes[activeFly][passiveFly].getEmpty<float>("distanceHeadTail");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				{	// calculate the angle to the other fly's centroid in [-180;180]
					float dX = passive_bodyCentroid[frameNumber].x() - active_bodyCentroid[frameNumber].x();
					float dY = passive_bodyCentroid[frameNumber].y() - active_bodyCentroid[frameNumber].y();
					float angle = std::atan2(dY, dX) * 180.0 / CV_PI;
					if (angle < 0) {
						angle += 360;
					}
					angleToOther.push_back(angleDifference(active_bodyOrientation[frameNumber], angle));
				}

				{	// calculate the distance between the head and the other fly's centroid
					double radAngle = active_bodyOrientation[frameNumber] * CV_PI / 180.;
					float headX = active_bodyCentroid[frameNumber].x() + (float)cos(radAngle) * 0.5f * active_bodyMajorAxisLength[frameNumber];
					float headY = active_bodyCentroid[frameNumber].y() + (float)sin(radAngle) * 0.5f * active_bodyMajorAxisLength[frameNumber];
					float dX = passive_bodyCentroid[frameNumber].x() - headX;
					float dY = passive_bodyCentroid[frameNumber].y() - headY;
					distanceHeadBody.push_back(std::sqrt(dX * dX + dY * dY));
				}

				{	// calculate the distance between the head and the other fly's tail
					double radAngle = active_bodyOrientation[frameNumber] * CV_PI / 180.;
					float headX = active_bodyCentroid[frameNumber].x() + (float)cos(radAngle) * 0.5f * active_bodyMajorAxisLength[frameNumber];
					float headY = active_bodyCentroid[frameNumber].y() + (float)sin(radAngle) * 0.5f * active_bodyMajorAxisLength[frameNumber];
					double radAngleOther = passive_bodyOrientation[frameNumber] * CV_PI / 180.;
					float tailX = passive_bodyCentroid[frameNumber].x() - (float)cos(radAngleOther) * 0.5f * passive_bodyMajorAxisLength[frameNumber];
					float tailY = passive_bodyCentroid[frameNumber].y() - (float)sin(radAngleOther) * 0.5f * passive_bodyMajorAxisLength[frameNumber];
					float dX = tailX - headX;
					float dY = tailY - headY;
					distanceHeadTail.push_back(std::sqrt(dX * dX + dY * dY));
				}

        { // calculate the angle subtended between the head and the other fly's head and tail
          float dX, dY;
					double radAngleSelf = active_bodyOrientation[frameNumber] * CV_PI / 180.;
					double radAngleOther = passive_bodyOrientation[frameNumber] * CV_PI / 180.;
					float refX = active_bodyCentroid[frameNumber].x() + (float)cos(radAngleSelf) * 0.5f * active_bodyMajorAxisLength[frameNumber];
					float refY = active_bodyCentroid[frameNumber].y() + (float)sin(radAngleSelf) * 0.5f * active_bodyMajorAxisLength[frameNumber];
					float headX = passive_bodyCentroid[frameNumber].x() + (float)cos(radAngleOther) * 0.5f * passive_bodyMajorAxisLength[frameNumber];
					float headY = passive_bodyCentroid[frameNumber].y() + (float)sin(radAngleOther) * 0.5f * passive_bodyMajorAxisLength[frameNumber];
					dX = headX - refX;
					dY = headY - refY;
					float headAngle = std::atan2(dY, dX) * 180.0 / CV_PI;
					float tailX = passive_bodyCentroid[frameNumber].x() - (float)cos(radAngleOther) * 0.5f * passive_bodyMajorAxisLength[frameNumber];
					float tailY = passive_bodyCentroid[frameNumber].y() - (float)sin(radAngleOther) * 0.5f * passive_bodyMajorAxisLength[frameNumber];
					dX = tailX - refX;
					dY = tailY - refY;
					float tailAngle = std::atan2(dY, dX) * 180.0 / CV_PI;
          float angle = fabs(headAngle-tailAngle);
					if (angle > 180) {
						angle = 360-angle;
					}
					angleSubtended.push_back(angle);
        }
			}
		}
	}

	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getFilled<float>("angleToOther");
			const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
			Attribute<Vf2>& vectorToOtherLocal = pairAttributes[activeFly][passiveFly].getEmpty<Vf2>("vectorToOtherLocal");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				float toRad = CV_PI / 180.0f;
				vectorToOtherLocal.push_back(makeVec(distanceBodyBody[frameNumber] * std::cos(angleToOther[frameNumber] * toRad), distanceBodyBody[frameNumber] * std::sin(angleToOther[frameNumber] * toRad)));
			}
		}
	}

	// calculate the change of the distance between the head and the other fly's centroid (which is used for "following")
	std::vector<float> deriveKernel;
	deriveKernel.push_back(-1);
	deriveKernel.push_back(1);
	std::vector<float> gaussKernel = discreteGaussian<float>(25);
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
			const Attribute<float>& distanceHeadTail = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceHeadTail");
			const Attribute<float>& distanceHeadBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceHeadBody");
			Attribute<float>& changeInDistanceBodyBody = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInDistanceBodyBody");
			Attribute<float>& changeInDistanceHeadTail = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInDistanceHeadTail");
			Attribute<float>& changeInDistanceHeadBody = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInDistanceHeadBody");
			changeInDistanceBodyBody.getData() = convolve_clamp(distanceBodyBody.getData(), deriveKernel);
			changeInDistanceHeadTail.getData() = convolve_clamp(distanceHeadTail.getData(), deriveKernel);
			changeInDistanceHeadBody.getData() = convolve_clamp(distanceHeadBody.getData(), deriveKernel);
			changeInDistanceBodyBody.getData() = convolve_clamp(changeInDistanceBodyBody.getData(), gaussKernel);	// make it smooth
			changeInDistanceHeadTail.getData() = convolve_clamp(changeInDistanceHeadTail.getData(), gaussKernel);	// make it smooth
			changeInDistanceHeadBody.getData() = convolve_clamp(changeInDistanceHeadBody.getData(), gaussKernel);	// make it smooth
		}
	}
}

void Arena::convertUnits()
{
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyAreaEccentricityCorrected = flyAttributes[flyNumber].getFilled<float>("bodyAreaEccentricityCorrected");
		Attribute<float>& bodyAreaEccentricityCorrected_u = flyAttributes[flyNumber].getEmpty<float>("bodyAreaEccentricityCorrected_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			bodyAreaEccentricityCorrected_u.push_back(bodyAreaEccentricityCorrected[frameNumber] / (pixelPerMillimeter * pixelPerMillimeter));
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& bodyCentroid = flyAttributes[flyNumber].getFilled<Vf2>("bodyCentroid");
		Attribute<Vf2>& bodyCentroid_u = flyAttributes[flyNumber].getEmpty<Vf2>("bodyCentroid_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			bodyCentroid_u.push_back(bodyCentroid[frameNumber] / pixelPerMillimeter);
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyMajorAxisLength = flyAttributes[flyNumber].getFilled<float>("bodyMajorAxisLength");
		Attribute<float>& bodyMajorAxisLength_u = flyAttributes[flyNumber].getEmpty<float>("bodyMajorAxisLength_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			bodyMajorAxisLength_u.push_back(bodyMajorAxisLength[frameNumber] / pixelPerMillimeter);
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyMinorAxisLength = flyAttributes[flyNumber].getFilled<float>("bodyMinorAxisLength");
		Attribute<float>& bodyMinorAxisLength_u = flyAttributes[flyNumber].getEmpty<float>("bodyMinorAxisLength_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			bodyMinorAxisLength_u.push_back(bodyMinorAxisLength[frameNumber] / pixelPerMillimeter);
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& distanceFromArenaCenter = flyAttributes[flyNumber].getFilled<float>("distanceFromArenaCenter");
		Attribute<float>& distanceFromArenaCenter_u = flyAttributes[flyNumber].getEmpty<float>("distanceFromArenaCenter_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			distanceFromArenaCenter_u.push_back(distanceFromArenaCenter[frameNumber] / pixelPerMillimeter);
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& filteredBodyCentroid = flyAttributes[flyNumber].getFilled<Vf2>("filteredBodyCentroid");
		Attribute<Vf2>& filteredBodyCentroid_u = flyAttributes[flyNumber].getEmpty<Vf2>("filteredBodyCentroid_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			filteredBodyCentroid_u.push_back(filteredBodyCentroid[frameNumber] / pixelPerMillimeter);
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<Vf2>& moved = flyAttributes[flyNumber].getFilled<Vf2>("moved");
		Attribute<Vf2>& moved_u = flyAttributes[flyNumber].getEmpty<Vf2>("moved_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			moved_u.push_back(moved[frameNumber] / pixelPerMillimeter);
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& movedAbs = flyAttributes[flyNumber].getFilled<float>("movedAbs");
		Attribute<float>& movedAbs_u = flyAttributes[flyNumber].getEmpty<float>("movedAbs_u");

		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			movedAbs_u.push_back(movedAbs[frameNumber] / pixelPerMillimeter);
		}
	}

	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
			const Attribute<float>& distanceHeadBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceHeadBody");
			const Attribute<float>& distanceHeadTail = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceHeadTail");
			const Attribute<float>& changeInDistanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("changeInDistanceBodyBody");
			const Attribute<float>& changeInDistanceHeadTail = pairAttributes[activeFly][passiveFly].getFilled<float>("changeInDistanceHeadTail");
			const Attribute<float>& changeInDistanceHeadBody = pairAttributes[activeFly][passiveFly].getFilled<float>("changeInDistanceHeadBody");
			const Attribute<Vf2>& vectorToOtherLocal = pairAttributes[activeFly][passiveFly].getFilled<Vf2>("vectorToOtherLocal");
			Attribute<float>& distanceBodyBody_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("distanceBodyBody_u");
			Attribute<float>& distanceHeadBody_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("distanceHeadBody_u");
			Attribute<float>& distanceHeadTail_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("distanceHeadTail_u");
			Attribute<float>& changeInDistanceBodyBody_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInDistanceBodyBody_u");
			Attribute<float>& changeInDistanceHeadTail_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInDistanceHeadTail_u");
			Attribute<float>& changeInDistanceHeadBody_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInDistanceHeadBody_u");
			Attribute<Vf2>& vectorToOtherLocal_u = pairAttributes[activeFly][passiveFly].getEmpty<Vf2>("vectorToOtherLocal_u");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				distanceBodyBody_u.push_back(distanceBodyBody[frameNumber] / pixelPerMillimeter);
				distanceHeadBody_u.push_back(distanceHeadBody[frameNumber] / pixelPerMillimeter);
				distanceHeadTail_u.push_back(distanceHeadTail[frameNumber] / pixelPerMillimeter);
				changeInDistanceBodyBody_u.push_back(changeInDistanceBodyBody[frameNumber] * sourceFrameRate / pixelPerMillimeter);
				changeInDistanceHeadTail_u.push_back(changeInDistanceHeadTail[frameNumber] * sourceFrameRate / pixelPerMillimeter);
				changeInDistanceHeadBody_u.push_back(changeInDistanceHeadBody[frameNumber] * sourceFrameRate / pixelPerMillimeter);
				vectorToOtherLocal_u.push_back(vectorToOtherLocal[frameNumber] / pixelPerMillimeter);
			}
		}
	}
}

void Arena::deriveCopulating(float medianFilterSeconds, float persistence)
{
	const Attribute<MyBool>& isOcclusion = frameAttributes.getFilled<MyBool>("isOcclusion");

	const size_t medianFilterWidth = roundToOdd(medianFilterSeconds * sourceFrameRate);
	const size_t erodeDilateWidth = roundToOdd(persistence * sourceFrameRate);
	// median filter isOcclusion
	std::vector<MyBool> copulating = ordfilt(isOcclusion.getData(), 0.5, medianFilterWidth);
	// erode/dilate copulating
	copulating = ordfilt(copulating, 0, erodeDilateWidth);
	copulating = ordfilt(copulating, 1, erodeDilateWidth);
	// use the same result for all flies
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		flyAttributes[flyNumber].getEmpty<MyBool>("copulating") = copulating;
	}
}
	
void Arena::deriveOrienting(float maxAngle, float minDistance, float maxDistance, float maxSpeedSelf, float maxSpeedOther, float medianFilterWidth, float persistence)
{
	const size_t oriAngleMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	minDistance *= pixelPerMillimeter;	// mm -> pixel
	maxDistance *= pixelPerMillimeter;	// mm -> pixel
	const size_t oriDistanceMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	maxSpeedSelf *= pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	maxSpeedOther *= pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	const size_t oriNotMovedMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t erodeDilateWidth = roundToOdd(persistence * sourceFrameRate);

	// speed thresholds
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& movedAbs = flyAttributes[flyNumber].getFilled<float>("movedAbs");
		Attribute<MyBool>& oriBelowMaxSpeedSelf = flyAttributes[flyNumber].getEmpty<MyBool>("oriBelowMaxSpeedSelf");
		Attribute<MyBool>& oriBelowMaxSpeedOther = flyAttributes[flyNumber].getEmpty<MyBool>("oriBelowMaxSpeedOther");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			oriBelowMaxSpeedSelf.push_back(movedAbs[frameNumber] < maxSpeedSelf);
			oriBelowMaxSpeedOther.push_back(movedAbs[frameNumber] < maxSpeedOther);
		}
		oriBelowMaxSpeedSelf.getData() = ordfilt(oriBelowMaxSpeedSelf.getData(), 0.5, oriNotMovedMedianFilterWidth);
		oriBelowMaxSpeedOther.getData() = ordfilt(oriBelowMaxSpeedOther.getData(), 0.5, oriNotMovedMedianFilterWidth);
	}

	// angle and distance
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<MyBool>& active_oriBelowMaxSpeedSelf = flyAttributes[activeFly].getFilled<MyBool>("oriBelowMaxSpeedSelf");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<MyBool>& passive_oriBelowMaxSpeedOther = flyAttributes[passiveFly].getFilled<MyBool>("oriBelowMaxSpeedOther");
			const Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getFilled<float>("angleToOther");
			const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
			Attribute<MyBool>& oriAngle = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("oriAngle");
			Attribute<MyBool>& oriDistance = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("oriDistance");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				oriAngle.push_back(std::abs(angleToOther[frameNumber]) < maxAngle);
				oriDistance.push_back(minDistance < distanceBodyBody[frameNumber] && distanceBodyBody[frameNumber] < maxDistance);
			}
			oriAngle.getData() = ordfilt(oriAngle.getData(), 0.5, oriAngleMedianFilterWidth);
			oriDistance.getData() = ordfilt(oriDistance.getData(), 0.5, oriDistanceMedianFilterWidth);

			// combine those three
			Attribute<MyBool>& orienting = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("orienting");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				orienting.push_back(oriAngle[frameNumber] && oriDistance[frameNumber] && active_oriBelowMaxSpeedSelf[frameNumber] && passive_oriBelowMaxSpeedOther[frameNumber]);
			}
			orienting.getData() = ordfilt(orienting.getData(), 0, erodeDilateWidth);
			orienting.getData() = ordfilt(orienting.getData(), 1, erodeDilateWidth);
		}
	}
}

void Arena::deriveRayEllipseOrienting(float growthOther, float maxAngle, float minDistance, float maxDistance, float maxSpeedSelf, float maxSpeedOther, float medianFilterWidth, float persistence)
{
	const size_t rayEllipseOriHitMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t oriAngleMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	minDistance *= pixelPerMillimeter;	// mm -> pixel
	maxDistance *= pixelPerMillimeter;	// mm -> pixel
	const size_t oriDistanceMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	maxSpeedSelf *= pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	maxSpeedOther *= pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	const size_t oriNotMovedMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t erodeDilateWidth = roundToOdd(persistence * sourceFrameRate);

	// speed thresholds
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& movedAbs = flyAttributes[flyNumber].getFilled<float>("movedAbs");
		Attribute<MyBool>& rayEllipseOriBelowMaxSpeedSelf = flyAttributes[flyNumber].getEmpty<MyBool>("rayEllipseOriBelowMaxSpeedSelf");
		Attribute<MyBool>& rayEllipseOriBelowMaxSpeedOther = flyAttributes[flyNumber].getEmpty<MyBool>("rayEllipseOriBelowMaxSpeedOther");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			rayEllipseOriBelowMaxSpeedSelf.push_back(movedAbs[frameNumber] < maxSpeedSelf);
			rayEllipseOriBelowMaxSpeedOther.push_back(movedAbs[frameNumber] < maxSpeedOther);
		}
		rayEllipseOriBelowMaxSpeedSelf.getData() = ordfilt(rayEllipseOriBelowMaxSpeedSelf.getData(), 0.5, oriNotMovedMedianFilterWidth);
		rayEllipseOriBelowMaxSpeedOther.getData() = ordfilt(rayEllipseOriBelowMaxSpeedOther.getData(), 0.5, oriNotMovedMedianFilterWidth);
	}

	// ray-ellipse intersection
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<Vf2>& active_bodyCentroid = flyAttributes[activeFly].getFilled<Vf2>("bodyCentroid");
		const Attribute<float>& active_bodyOrientation = flyAttributes[activeFly].getFilled<float>("bodyOrientation");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<Vf2>& passive_bodyCentroid = flyAttributes[passiveFly].getFilled<Vf2>("bodyCentroid");
			const Attribute<float>& passive_bodyMajorAxisLength = flyAttributes[passiveFly].getFilled<float>("bodyMajorAxisLength");
			const Attribute<float>& passive_bodyMinorAxisLength = flyAttributes[passiveFly].getFilled<float>("bodyMinorAxisLength");
			const Attribute<float>& passive_bodyOrientation = flyAttributes[passiveFly].getFilled<float>("bodyOrientation");
			Attribute<MyBool>& rayEllipseOriHit = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("rayEllipseOriHit");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				// ray-ellipse intersection
				float otherHalfMajor = passive_bodyMajorAxisLength[frameNumber] * growthOther * 0.5;
				float otherHalfMinor = passive_bodyMinorAxisLength[frameNumber] * growthOther * 0.5;
				float otherOrientation = passive_bodyOrientation[frameNumber];
				// rotate self's view vector into the ellipse's local coordinate system of other
				float selfOrientation = active_bodyOrientation[frameNumber] - otherOrientation;
				float selfViewX = (float)cos(-selfOrientation * CV_PI / 180.);
				float selfViewY = -(float)sin(-selfOrientation * CV_PI / 180.);	// - because our coordinate system has positive y pointing down
				// translate self's position into the ellipse's local coordinate system of other
				float selfX = active_bodyCentroid[frameNumber].x() - passive_bodyCentroid[frameNumber].x();
				float selfY = active_bodyCentroid[frameNumber].y() - passive_bodyCentroid[frameNumber].y();
				// rotate self's position into the ellipse's local coordinate system of other
				float c = (float)cos(-otherOrientation * CV_PI / 180.);
				float s = (float)sin(-otherOrientation * CV_PI / 180.);
				float rotatedSelfX = selfX * c - selfY * s;
				float rotatedSelfY = selfX * s + selfY * c;
				// we're plugging the parametric view ray description (self + d * selfView) into the implicit ellipse equation
				// we could solve for the parameter d (...the distance of the intersection)
				// but it's enough to determine whether the intersection exists...
				float quadraticEqA = selfViewX * selfViewX * otherHalfMinor * otherHalfMinor + selfViewY * selfViewY * otherHalfMajor * otherHalfMajor;
				float quadraticEqB = 2 * selfViewX * rotatedSelfX * otherHalfMinor * otherHalfMinor + 2 * selfViewY * rotatedSelfY * otherHalfMajor * otherHalfMajor;
				float quadraticEqC = rotatedSelfX * rotatedSelfX * otherHalfMinor * otherHalfMinor + rotatedSelfY * rotatedSelfY * otherHalfMajor * otherHalfMajor - otherHalfMajor * otherHalfMajor * otherHalfMinor * otherHalfMinor;
				float discriminant = quadraticEqB * quadraticEqB - 4 * quadraticEqA * quadraticEqC;
				if (discriminant < 0) {	// no intersection
					rayEllipseOriHit.push_back(false);
				} else {
					// ...and whether the distance of the far intersection is positive (i.e. in front of the fly)
					if (sqrt(discriminant) - quadraticEqB < 0) {
						rayEllipseOriHit.push_back(false);
					} else {
						rayEllipseOriHit.push_back(true);
					}
				}
			}
			rayEllipseOriHit.getData() = ordfilt(rayEllipseOriHit.getData(), 0.5, rayEllipseOriHitMedianFilterWidth);
		}
	}

	// angle and distance
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<MyBool>& active_rayEllipseOriBelowMaxSpeedSelf = flyAttributes[activeFly].getFilled<MyBool>("rayEllipseOriBelowMaxSpeedSelf");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<MyBool>& passive_rayEllipseOriBelowMaxSpeedOther = flyAttributes[passiveFly].getFilled<MyBool>("rayEllipseOriBelowMaxSpeedOther");
			const Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getFilled<float>("angleToOther");
			const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
			Attribute<MyBool>& rayEllipseOriAngle = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("rayEllipseOriAngle");
			Attribute<MyBool>& rayEllipseOriDistance = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("rayEllipseOriDistance");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				rayEllipseOriAngle.push_back(std::abs(angleToOther[frameNumber]) < maxAngle);
				rayEllipseOriDistance.push_back(minDistance < distanceBodyBody[frameNumber] && distanceBodyBody[frameNumber] < maxDistance);
			}
			rayEllipseOriAngle.getData() = ordfilt(rayEllipseOriAngle.getData(), 0.5, oriAngleMedianFilterWidth);
			rayEllipseOriDistance.getData() = ordfilt(rayEllipseOriDistance.getData(), 0.5, oriDistanceMedianFilterWidth);

			// combine those four
			const Attribute<MyBool>& rayEllipseOriHit = pairAttributes[activeFly][passiveFly].getFilled<MyBool>("rayEllipseOriHit");
			Attribute<MyBool>& rayEllipseOrienting = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("rayEllipseOrienting");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				rayEllipseOrienting.push_back(rayEllipseOriHit[frameNumber] && rayEllipseOriAngle[frameNumber] && rayEllipseOriDistance[frameNumber] && active_rayEllipseOriBelowMaxSpeedSelf[frameNumber] && passive_rayEllipseOriBelowMaxSpeedOther[frameNumber]);
			}
			rayEllipseOrienting.getData() = ordfilt(rayEllipseOrienting.getData(), 0, erodeDilateWidth);
			rayEllipseOrienting.getData() = ordfilt(rayEllipseOrienting.getData(), 1, erodeDilateWidth);
		}
	}
}
	
void Arena::deriveFollowing(float maxChangeOfDistance, float maxAngle, float minDistance, float maxDistance, float minSpeed, float maxMovementDirectionDifference, float medianFilterWidth, float persistence)
{
	const float maxChangeInDistanceHeadBody = maxChangeOfDistance * pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	const size_t follSmallChangeInDistanceMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t follAngleMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	minDistance *= pixelPerMillimeter;	// mm -> pixel
	maxDistance *= pixelPerMillimeter;	// mm -> pixel
	const size_t follDistanceMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const float movedMin = minSpeed * pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	const size_t follMovedMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t follSameMovedDirectionMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t follBehindMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t erodeDilateWidth = roundToOdd(persistence * sourceFrameRate);
				
	// speed thresholds
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& movedAbs = flyAttributes[flyNumber].getFilled<float>("movedAbs");
		Attribute<MyBool>& follAboveMinSpeedSelf = flyAttributes[flyNumber].getEmpty<MyBool>("follAboveMinSpeedSelf");
		Attribute<MyBool>& follAboveMinSpeedOther = flyAttributes[flyNumber].getEmpty<MyBool>("follAboveMinSpeedOther");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			follAboveMinSpeedSelf.push_back(movedAbs[frameNumber] > movedMin);
			follAboveMinSpeedOther.push_back(movedAbs[frameNumber] > movedMin);
		}
		follAboveMinSpeedSelf.getData() = ordfilt(follAboveMinSpeedSelf.getData(), 0.5, follMovedMedianFilterWidth);
		follAboveMinSpeedOther.getData() = ordfilt(follAboveMinSpeedOther.getData(), 0.5, follMovedMedianFilterWidth);
	}

	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<MyBool>& active_follAboveMinSpeedSelf = flyAttributes[activeFly].getFilled<MyBool>("follAboveMinSpeedSelf");
		const Attribute<float>& active_movedDirectionGlobal = flyAttributes[activeFly].getFilled<float>("movedDirectionGlobal");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<MyBool>& passive_follAboveMinSpeedOther = flyAttributes[passiveFly].getFilled<MyBool>("follAboveMinSpeedOther");
			const Attribute<float>& passive_movedDirectionGlobal = flyAttributes[passiveFly].getFilled<float>("movedDirectionGlobal");
			const Attribute<float>& changeInDistanceHeadBody = pairAttributes[activeFly][passiveFly].getFilled<float>("changeInDistanceHeadBody");
			const Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getFilled<float>("angleToOther");
			const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
			const Attribute<float>& active_distanceHeadTail = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceHeadTail");
			const Attribute<float>& passive_distanceHeadTail = pairAttributes[passiveFly][activeFly].getFilled<float>("distanceHeadTail");
			Attribute<MyBool>& follSmallChangeInDistance = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("follSmallChangeInDistance");
			Attribute<MyBool>& follAngle = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("follAngle");
			Attribute<MyBool>& follDistance = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("follDistance");
			Attribute<MyBool>& follSameMovedDirection = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("follSameMovedDirection");
			Attribute<MyBool>& follBehind = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("follBehind");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				follSmallChangeInDistance.push_back(std::abs(changeInDistanceHeadBody[frameNumber]) < maxChangeInDistanceHeadBody);
				follAngle.push_back(std::abs(angleToOther[frameNumber]) < maxAngle);
				follDistance.push_back(minDistance < distanceBodyBody[frameNumber] && distanceBodyBody[frameNumber] < maxDistance);
				follSameMovedDirection.push_back(std::abs(angleDifference(active_movedDirectionGlobal[frameNumber], passive_movedDirectionGlobal[frameNumber])) < maxMovementDirectionDifference);
				follBehind.push_back(active_distanceHeadTail[frameNumber] < passive_distanceHeadTail[frameNumber]);
			}
			follSmallChangeInDistance.getData() = ordfilt(follSmallChangeInDistance.getData(), 0.5, follSmallChangeInDistanceMedianFilterWidth);
			follAngle.getData() = ordfilt(follAngle.getData(), 0.5, follAngleMedianFilterWidth);
			follDistance.getData() = ordfilt(follDistance.getData(), 0.5, follDistanceMedianFilterWidth);
			follSameMovedDirection.getData() = ordfilt(follSameMovedDirection.getData(), 0.5, follSameMovedDirectionMedianFilterWidth);
			follBehind.getData() = ordfilt(follBehind.getData(), 0.5, follBehindMedianFilterWidth);

			// combine those six
			Attribute<MyBool>& following = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("following");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				following.push_back(
					follSmallChangeInDistance[frameNumber] &&
					follAngle[frameNumber] &&
					follDistance[frameNumber] &&
					follSameMovedDirection[frameNumber] &&
					follBehind[frameNumber] &&
					active_follAboveMinSpeedSelf[frameNumber] &&
					passive_follAboveMinSpeedOther[frameNumber]
				);
			}
			following.getData() = ordfilt(following.getData(), 0, erodeDilateWidth);
			following.getData() = ordfilt(following.getData(), 1, erodeDilateWidth);
		}
	}

	std::vector<std::string> occurredPairAttributes;
	occurredPairAttributes.push_back("following");
	for (std::vector<std::string>::const_iterator iter = occurredPairAttributes.begin(); iter != occurredPairAttributes.end(); ++iter) {
		for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
			for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
				if (activeFly == passiveFly) {
					continue;
				}
				const Attribute<MyBool>& attrib = pairAttributes[activeFly][passiveFly].getFilled<MyBool>(*iter);
				Attribute<MyBool>& attribOccurred = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>((*iter) + "Occurred");
				attribOccurred.getData() = attrib.getData();
				std::vector<MyBool>::iterator first = std::find(attribOccurred.begin(), attribOccurred.end(), true);
				std::fill(first, attribOccurred.end(), true);
			}
		}
	}
}

void Arena::deriveCircling(float minDistance, float maxDistance, float maxAngle, float minSpeedSelf, float maxSpeedOther, float minAngleDifference, float minSidewaysSpeed, float medianFilterWidth, float persistence)
{
	minDistance *= pixelPerMillimeter;	// mm -> pixel
	maxDistance *= pixelPerMillimeter;	// mm -> pixel
	const size_t circDistanceMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t circAngleMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const float thisMovedMin = minSpeedSelf * pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	const size_t circThisMovedMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const float otherMovedMax = maxSpeedOther * pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	const size_t circOtherNotMovedMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t circMovedSidewaysMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	minSidewaysSpeed *= pixelPerMillimeter / sourceFrameRate;	// mm/s -> pixel/frame
	const size_t circAboveMinSidewaysSpeedMedianFilterWidth = roundToOdd(medianFilterWidth * sourceFrameRate);
	const size_t erodeDilateWidth = roundToOdd(persistence * sourceFrameRate);

	// speed thresholds
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& movedAbs = flyAttributes[flyNumber].getFilled<float>("movedAbs");
		const Attribute<float>& movedDirectionLocal = flyAttributes[flyNumber].getFilled<float>("movedDirectionLocal");
		Attribute<MyBool>& circAboveMinSpeedSelf = flyAttributes[flyNumber].getEmpty<MyBool>("circAboveMinSpeedSelf");
		Attribute<MyBool>& circBelowMaxSpeedOther = flyAttributes[flyNumber].getEmpty<MyBool>("circBelowMaxSpeedOther");
		Attribute<MyBool>& circMovedSideways = flyAttributes[flyNumber].getEmpty<MyBool>("circMovedSideways");
		Attribute<MyBool>& circAboveMinSidewaysSpeed = flyAttributes[flyNumber].getEmpty<MyBool>("circAboveMinSidewaysSpeed");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			circAboveMinSpeedSelf.push_back(movedAbs[frameNumber] > thisMovedMin);
			circBelowMaxSpeedOther.push_back(movedAbs[frameNumber] < otherMovedMax);
			circMovedSideways.push_back(std::abs(movedDirectionLocal[frameNumber]) > minAngleDifference);
			circAboveMinSidewaysSpeed.push_back(std::abs(std::sin(movedDirectionLocal[frameNumber] * CV_PI / 180.0)) * movedAbs[frameNumber] > minSidewaysSpeed);
		}
		circAboveMinSpeedSelf.getData() = ordfilt(circAboveMinSpeedSelf.getData(), 0.5, circThisMovedMedianFilterWidth);
		circBelowMaxSpeedOther.getData() = ordfilt(circBelowMaxSpeedOther.getData(), 0.5, circOtherNotMovedMedianFilterWidth);
		circMovedSideways.getData() = ordfilt(circMovedSideways.getData(), 0.5, circMovedSidewaysMedianFilterWidth);
		circAboveMinSidewaysSpeed.getData() = ordfilt(circAboveMinSidewaysSpeed.getData(), 0.5, circAboveMinSidewaysSpeedMedianFilterWidth);
	}

	// angle and distance
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<MyBool>& active_circAboveMinSpeedSelf = flyAttributes[activeFly].getFilled<MyBool>("circAboveMinSpeedSelf");
		const Attribute<MyBool>& active_circMovedSideways = flyAttributes[activeFly].getFilled<MyBool>("circMovedSideways");
		const Attribute<MyBool>& active_circAboveMinSidewaysSpeed = flyAttributes[activeFly].getFilled<MyBool>("circAboveMinSidewaysSpeed");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<MyBool>& passive_circBelowMaxSpeedOther = flyAttributes[passiveFly].getFilled<MyBool>("circBelowMaxSpeedOther");
			const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
			const Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getFilled<float>("angleToOther");
			Attribute<MyBool>& circAngle = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("circAngle");
			Attribute<MyBool>& circDistance = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("circDistance");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				circAngle.push_back(std::abs(angleToOther[frameNumber]) < maxAngle);
				circDistance.push_back(minDistance < distanceBodyBody[frameNumber] && distanceBodyBody[frameNumber] < maxDistance);
			}
			circAngle.getData() = ordfilt(circAngle.getData(), 0.5, circAngleMedianFilterWidth);
			circDistance.getData() = ordfilt(circDistance.getData(), 0.5, circDistanceMedianFilterWidth);

			// combine those four
			Attribute<MyBool>& circling = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("circling");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				circling.push_back(
					circAngle[frameNumber] &&
					circDistance[frameNumber] &&
					active_circAboveMinSpeedSelf[frameNumber] &&
					active_circMovedSideways[frameNumber] &&
					active_circAboveMinSidewaysSpeed[frameNumber] &&
					passive_circBelowMaxSpeedOther[frameNumber]
				);
			}
			circling.getData() = ordfilt(circling.getData(), 0, erodeDilateWidth);
			circling.getData() = ordfilt(circling.getData(), 1, erodeDilateWidth);
		}
	}
}

void Arena::deriveWingExt(float minAngle, float tailQuadrantAreaRatio, float directionTolerance, float minBoc, float angleMedianFilterWidth, float areaMedianFilterWidth, float persistence)
{
	const size_t wingExtAngleMedianFilterWidth = roundToOdd(angleMedianFilterWidth * sourceFrameRate);
	const float minAreaRatio = tailQuadrantAreaRatio;	// (wing quadrant area) / (body area)
	const size_t wingExtAreaMedianFilterWidth = roundToOdd(areaMedianFilterWidth * sourceFrameRate);
	const float maxFacingAngle = directionTolerance;	// if the other fly is that much to the left or right, it's wingExtTowards
	const size_t erodeDilateWidth = roundToOdd(persistence * sourceFrameRate);

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<float>& bodyArea = flyAttributes[flyNumber].getFilled<float>("bodyArea");
		const Attribute<float>& leftWingAngle = flyAttributes[flyNumber].getFilled<float>("leftWingAngle");
		const Attribute<float>& rightWingAngle = flyAttributes[flyNumber].getFilled<float>("rightWingAngle");
		const Attribute<float>& leftBodyArea = flyAttributes[flyNumber].getFilled<float>("leftBodyArea");
		const Attribute<float>& rightBodyArea = flyAttributes[flyNumber].getFilled<float>("rightBodyArea");
		const Attribute<float>& leftWingArea = flyAttributes[flyNumber].getFilled<float>("leftWingArea");
		const Attribute<float>& rightWingArea = flyAttributes[flyNumber].getFilled<float>("rightWingArea");
		Attribute<MyBool>& wingExtAngleLeft = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtAngleLeft");
		Attribute<MyBool>& wingExtAngleRight = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtAngleRight");
		Attribute<MyBool>& wingExtAreaLeft = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtAreaLeft");
		Attribute<MyBool>& wingExtAreaRight = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtAreaRight");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			wingExtAngleLeft.push_back(leftWingAngle[frameNumber] > minAngle);
			wingExtAngleRight.push_back(rightWingAngle[frameNumber] > minAngle);
			wingExtAreaLeft.push_back(leftWingArea[frameNumber] - leftBodyArea[frameNumber] > minAreaRatio * bodyArea[frameNumber]);
			wingExtAreaRight.push_back(rightWingArea[frameNumber] - rightBodyArea[frameNumber] > minAreaRatio * bodyArea[frameNumber]);
		}
		wingExtAngleLeft.getData() = ordfilt(wingExtAngleLeft.getData(), 0.5, wingExtAngleMedianFilterWidth);
		wingExtAngleRight.getData() = ordfilt(wingExtAngleRight.getData(), 0.5, wingExtAngleMedianFilterWidth);
		wingExtAreaLeft.getData() = ordfilt(wingExtAreaLeft.getData(), 0.5, wingExtAreaMedianFilterWidth);
		wingExtAreaRight.getData() = ordfilt(wingExtAreaRight.getData(), 0.5, wingExtAreaMedianFilterWidth);
	}

	const Attribute<float>& tBoc = frameAttributes.getFilled<float>("tBoc");
	const Attribute<MyBool>& isOcclusion = frameAttributes.getFilled<MyBool>("isOcclusion");
	Attribute<MyBool>& wingExtCallableDuringOcclusion = frameAttributes.getEmpty<MyBool>("wingExtCallableDuringOcclusion");
	for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
		wingExtCallableDuringOcclusion.push_back(!isOcclusion[frameNumber] || tBoc[frameNumber] > minBoc);
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<MyBool>& wingExtAngleLeft = flyAttributes[flyNumber].getFilled<MyBool>("wingExtAngleLeft");
		const Attribute<MyBool>& wingExtAngleRight = flyAttributes[flyNumber].getFilled<MyBool>("wingExtAngleRight");
		const Attribute<MyBool>& wingExtAreaLeft = flyAttributes[flyNumber].getFilled<MyBool>("wingExtAreaLeft");
		const Attribute<MyBool>& wingExtAreaRight = flyAttributes[flyNumber].getFilled<MyBool>("wingExtAreaRight");
		Attribute<MyBool>& wingExtLeft = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtLeft");
		Attribute<MyBool>& wingExtRight = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtRight");
		Attribute<MyBool>& wingExt = flyAttributes[flyNumber].getEmpty<MyBool>("wingExt");
		Attribute<MyBool>& wingExtBoth = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtBoth");
		Attribute<MyBool>& wingExtEitherOr = flyAttributes[flyNumber].getEmpty<MyBool>("wingExtEitherOr");
		for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
			wingExtLeft.push_back(wingExtAngleLeft[frameNumber] && wingExtAreaLeft[frameNumber] && wingExtCallableDuringOcclusion[frameNumber]);
			wingExtRight.push_back(wingExtAngleRight[frameNumber] && wingExtAreaRight[frameNumber] && wingExtCallableDuringOcclusion[frameNumber]);
			wingExt.push_back(wingExtLeft.back() || wingExtRight.back());
			wingExtBoth.push_back(wingExtLeft.back() && wingExtRight.back());
			wingExtEitherOr.push_back(wingExtLeft.back() != wingExtRight.back());
		}
		wingExtLeft.getData() = ordfilt(wingExtLeft.getData(), 0, erodeDilateWidth);
		wingExtLeft.getData() = ordfilt(wingExtLeft.getData(), 1, erodeDilateWidth);
		wingExtRight.getData() = ordfilt(wingExtRight.getData(), 0, erodeDilateWidth);
		wingExtRight.getData() = ordfilt(wingExtRight.getData(), 1, erodeDilateWidth);
		wingExt.getData() = ordfilt(wingExt.getData(), 0, erodeDilateWidth);
		wingExt.getData() = ordfilt(wingExt.getData(), 1, erodeDilateWidth);
		wingExtBoth.getData() = ordfilt(wingExtBoth.getData(), 0, erodeDilateWidth);
		wingExtBoth.getData() = ordfilt(wingExtBoth.getData(), 1, erodeDilateWidth);
		wingExtEitherOr.getData() = ordfilt(wingExtEitherOr.getData(), 0, erodeDilateWidth);
		wingExtEitherOr.getData() = ordfilt(wingExtEitherOr.getData(), 1, erodeDilateWidth);
	}

	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		const Attribute<MyBool>& active_wingExtLeft = flyAttributes[activeFly].getFilled<MyBool>("wingExtLeft");
		const Attribute<MyBool>& active_wingExtRight = flyAttributes[activeFly].getFilled<MyBool>("wingExtRight");
		const Attribute<MyBool>& active_wingExt = flyAttributes[activeFly].getFilled<MyBool>("wingExt");
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getFilled<float>("angleToOther");
			Attribute<MyBool>& wingExtTowards = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("wingExtTowards");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				wingExtTowards.push_back(
					(std::abs(angleToOther[frameNumber]) < maxFacingAngle && active_wingExt[frameNumber]) ||
					(angleToOther[frameNumber] < 0 && active_wingExtLeft[frameNumber]) ||
					(angleToOther[frameNumber] > 0 && active_wingExtRight[frameNumber])
				);
			}
			Attribute<MyBool>& wingExtAway = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("wingExtAway");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				wingExtAway.push_back(
					(std::abs(angleToOther[frameNumber]) < maxFacingAngle && active_wingExt[frameNumber]) ||
					(angleToOther[frameNumber] < 0 && active_wingExtRight[frameNumber]) ||
					(angleToOther[frameNumber] > 0 && active_wingExtLeft[frameNumber])
				);
			}
			wingExtTowards.getData() = ordfilt(wingExtTowards.getData(), 0, erodeDilateWidth);
			wingExtTowards.getData() = ordfilt(wingExtTowards.getData(), 1, erodeDilateWidth);
			wingExtAway.getData() = ordfilt(wingExtAway.getData(), 0, erodeDilateWidth);
			wingExtAway.getData() = ordfilt(wingExtAway.getData(), 1, erodeDilateWidth);
		}
	}

	std::vector<std::string> occurredFlyAttributes;
	occurredFlyAttributes.push_back("wingExt");
	occurredFlyAttributes.push_back("wingExtLeft");
	occurredFlyAttributes.push_back("wingExtRight");
	for (std::vector<std::string>::const_iterator iter = occurredFlyAttributes.begin(); iter != occurredFlyAttributes.end(); ++iter) {
		for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
			const Attribute<MyBool>& attrib = flyAttributes[flyNumber].getFilled<MyBool>(*iter);
			Attribute<MyBool>& attribOccurred = flyAttributes[flyNumber].getEmpty<MyBool>((*iter) + "Occurred");
			attribOccurred.getData() = attrib.getData();
			std::vector<MyBool>::iterator first = std::find(attribOccurred.begin(), attribOccurred.end(), true);
			std::fill(first, attribOccurred.end(), true);
		}
	}
}

void Arena::deriveCourtship(float circlingWeight, float copulatingWeight, float followingWeight, float orientingWeight, float rayEllipseOrientingWeight, float wingExtWeight)
{
	{	// weightedCourting
		for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
			const Attribute<MyBool>& copulating = flyAttributes[activeFly].getFilled<MyBool>("copulating");
			const Attribute<MyBool>& wingExt = flyAttributes[activeFly].getFilled<MyBool>("wingExt");
			Attribute<float>& weightedCourting = flyAttributes[activeFly].getEmpty<float>("weightedCourting");
			weightedCourting.resize(getFrameCount());
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				weightedCourting[frameNumber] += wingExt[frameNumber] * wingExtWeight;
				weightedCourting[frameNumber] += copulating[frameNumber] * copulatingWeight;
			}
			for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
				if (activeFly == passiveFly) {
					continue;
				}
				const Attribute<MyBool>& circling = pairAttributes[activeFly][passiveFly].getFilled<MyBool>("circling");
				const Attribute<MyBool>& following = pairAttributes[activeFly][passiveFly].getFilled<MyBool>("following");
				const Attribute<MyBool>& orienting = pairAttributes[activeFly][passiveFly].getFilled<MyBool>("orienting");
				const Attribute<MyBool>& rayEllipseOrienting = pairAttributes[activeFly][passiveFly].getFilled<MyBool>("rayEllipseOrienting");
				for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
					weightedCourting[frameNumber] += circling[frameNumber] * circlingWeight;
					weightedCourting[frameNumber] += following[frameNumber] * followingWeight;
					weightedCourting[frameNumber] += orienting[frameNumber] * orientingWeight;
					weightedCourting[frameNumber] += rayEllipseOrienting[frameNumber] * rayEllipseOrientingWeight;
				}
			}
		}
	}

	{	// courting
		for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
			const Attribute<float>& weightedCourting = flyAttributes[flyNumber].getFilled<float>("weightedCourting");
			Attribute<MyBool>& courting = flyAttributes[flyNumber].getEmpty<MyBool>("courting");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				courting.push_back(weightedCourting[frameNumber] > 0.01);	//TODO: arbitrary cutoff...make it an option?
			}
		}
	}

	{	// courtship (as a per-frame attribute)
		Attribute<MyBool>& courtship = frameAttributes.getEmpty<MyBool>("courtship");
		courtship.resize(getFrameCount());
		for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
			const Attribute<MyBool>& courting = flyAttributes[flyNumber].getFilled<MyBool>("courting");
			for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
				courtship[frameNumber] = courtship[frameNumber] || courting[frameNumber];
			}
		}
	}
}

void Arena::deriveNew()
{
  {
    std::vector<float> deriveKernel;
    deriveKernel.push_back(-1);
    deriveKernel.push_back(1);
    std::vector<float> gaussKernel = discreteGaussian<float>(25);
		for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
			const Attribute<MyBool>& wingExt = flyAttributes[activeFly].getFilled<MyBool>("wingExt");
			const Attribute<MyBool>& wingExtLeft = flyAttributes[activeFly].getFilled<MyBool>("wingExtLeft");
			const Attribute<MyBool>& wingExtRight = flyAttributes[activeFly].getFilled<MyBool>("wingExtRight");
      for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
        if (activeFly == passiveFly) {
          continue;
        }
        const Attribute<float>& angleToOther = pairAttributes[activeFly][passiveFly].getFilled<float>("angleToOther");
        const Attribute<float>& angleSubtended = pairAttributes[activeFly][passiveFly].getFilled<float>("angleSubtended");
        Attribute<MyBool>& wingExtFront = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("wingExtFront");
        Attribute<MyBool>& wingExtIpsi = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("wingExtIpsi");
        Attribute<MyBool>& wingExtContra = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("wingExtContra");
        Attribute<MyBool>& wingExtBehind = pairAttributes[activeFly][passiveFly].getEmpty<MyBool>("wingExtBehind");
        Attribute<float>& changeInAngleToOther = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInAngleToOther");
        Attribute<float>& changeInAngleToOther_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInAngleToOther_u");
        Attribute<float>& changeInAngleSubtended = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInAngleSubtended");
        Attribute<float>& changeInAngleSubtended_u = pairAttributes[activeFly][passiveFly].getEmpty<float>("changeInAngleSubtended_u");
        changeInAngleToOther.getData()   = convolve_clamp(angleToOther.getData(), deriveKernel);
        changeInAngleSubtended.getData() = convolve_clamp(angleSubtended.getData(), deriveKernel);
        changeInAngleToOther.getData()   = convolve_clamp(changeInAngleToOther.getData(), gaussKernel);	// make it smooth
        changeInAngleSubtended.getData() = convolve_clamp(changeInAngleSubtended.getData(), gaussKernel);	// make it smooth
        for (size_t frameNumber = 0; frameNumber != getFrameCount(); ++frameNumber) {
          wingExtFront.push_back(wingExt[frameNumber] && fabs(angleToOther[frameNumber])<15);
          wingExtIpsi.push_back(wingExtLeft[frameNumber] && angleToOther[frameNumber]<-15 && angleToOther[frameNumber]>-105);
          wingExtContra.push_back(wingExtRight[frameNumber] && angleToOther[frameNumber]>15 && angleToOther[frameNumber]<105);
          wingExtBehind.push_back(wingExt[frameNumber] && fabs(angleToOther[frameNumber])>105);
          changeInAngleToOther_u.push_back(changeInAngleToOther[frameNumber] * sourceFrameRate);
          changeInAngleSubtended_u.push_back(changeInAngleSubtended[frameNumber] * sourceFrameRate);
        }
		  }
		}
	}
}

void Arena::importTrackingData(std::istream& in)
{
	flyAttributes.resize(getFlyCount());
	pairAttributes.resize(getFlyCount());
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		pairAttributes[activeFly].resize(getFlyCount());
	}

	std::string table;
	in.seekg(0, std::ios::end);   
	table.reserve(in.tellg());
	in.seekg(0, std::ios::beg);
	table.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());	// extra parentheses are required: "most vexing parse"
	table = ::transpose(table);

	std::istringstream attributeStream(table);
	const char delimiter = '\t';
	size_t lineNumber = 0;
	for (std::string line; std::getline(attributeStream, line); ++lineNumber) {
		std::vector<std::string> splitLine = ::split(line);
		if (splitLine.size() < 4) {
			std::cerr << "importTrackingData: column " << lineNumber << " is not valid; skipping...\n";
			continue;
		}
		if (splitLine[0] == "") {	// frame attribute
			if (frameAttributes.has(splitLine[1])) {
				frameAttributes.getEmpty(splitLine[1]).read(splitLine.begin() + 4, splitLine.end());
			} else {
				std::cerr << "importTrackingData: unknown attribute name in column " << lineNumber << "; skipping...\n";
				continue;
			}
		} else {	// fly attribute or pair attribute
			std::vector<std::string> splitFlyNumber = ::split(splitLine[0], ' ');
			if (splitFlyNumber.size() == 1) {	// fly attribute
				size_t flyNumber = 0;
				try {
					flyNumber = unstringify<size_t>(splitFlyNumber[0]);
				} catch (std::runtime_error&) {
					std::cerr << "importTrackingData: unknown fly number in column " << lineNumber << "; skipping...\n";
					continue;
				}
				if (flyNumber >= getFlyCount()) {
					std::cerr << "importTrackingData: fly number out of range in column " << lineNumber << "; skipping...\n";
					continue;
				}
				if (flyAttributes[flyNumber].has(splitLine[1])) {
					flyAttributes[flyNumber].getEmpty(splitLine[1]).read(splitLine.begin() + 4, splitLine.end());
				} else {
					std::cerr << "importTrackingData: unknown attribute name in column " << lineNumber << "; skipping...\n";
					continue;
				}
			} else if (splitFlyNumber.size() == 2) {	// pair attribute
				size_t activeNumber = 0;
				size_t passiveNumber = 0;
				try {
					activeNumber = unstringify<size_t>(splitFlyNumber[0]);
					passiveNumber = unstringify<size_t>(splitFlyNumber[1]);
				} catch (std::runtime_error&) {
					std::cerr << "importTrackingData: unknown fly number in column " << lineNumber << "; skipping...\n";
					continue;
				}
				if (activeNumber >= getFlyCount() || passiveNumber >= getFlyCount()) {
					std::cerr << "importTrackingData: fly number out of range in column " << lineNumber << "; skipping...\n";
					continue;
				}
				if (pairAttributes[activeNumber][passiveNumber].has(splitLine[1])) {
					pairAttributes[activeNumber][passiveNumber].getEmpty(splitLine[1]).read(splitLine.begin() + 4, splitLine.end());
				} else {
					std::cerr << "importTrackingData: unknown attribute name in column " << lineNumber << "; skipping...\n";
					continue;
				}
			} else {
				std::cerr << "importTrackingData: unknown attribute type in column " << lineNumber << "; skipping...\n";
				continue;
			}
		}
	}

	// clear attributes that shouldn't be available at this point
	frameAttributes.clearDerivedAttributes();
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		flyAttributes[flyNumber].clearDerivedAttributes();
	}
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			pairAttributes[activeFly][passiveFly].clearDerivedAttributes();
		}
	}
}

void Arena::exportTrackingData(std::ostream& out) const
{
	const char delimiter = '\t';
	std::ostringstream trans;
	frameAttributes.write(trans);
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		flyAttributes[flyNumber].write(trans, stringify(flyNumber));
	}
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			pairAttributes[activeFly][passiveFly].write(trans, stringify(activeFly) + " " + stringify(passiveFly));
		}
	}
//	out << trans.str();
	out << ::transpose(trans.str());
}

void Arena::exportAttributes(const std::string& outDirPath) const
{
	std::string frameDirPath = outDirPath + "/frame";
	makeDirectory(frameDirPath);
	frameAttributes.writeBinaries(frameDirPath);

	std::string flyDirPath = outDirPath + "/fly";
	makeDirectory(flyDirPath);
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		std::string thisFlyDirPath = flyDirPath + "/" + stringify(flyNumber);
		makeDirectory(thisFlyDirPath);
		flyAttributes[flyNumber].writeBinaries(thisFlyDirPath);
	}

	std::string pairDirPath = outDirPath + "/pair";
	makeDirectory(pairDirPath);
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		std::string activeDirPath = pairDirPath + "/" + stringify(activeFly);
		makeDirectory(activeDirPath);
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			std::string passiveDirPath = activeDirPath + "/" + stringify(passiveFly);
			makeDirectory(passiveDirPath);
			pairAttributes[activeFly][passiveFly].writeBinaries(passiveDirPath);
		}
	}
}

size_t Arena::writeContour(const std::vector<std::vector<cv::Point> >& contour)
{
	const size_t currentOffset = contourFileOffset;
	const uint32_t segmentCount = contour.size();
	contourFile->write((char*)&segmentCount, sizeof(segmentCount)); contourFileOffset += sizeof(segmentCount);
	for (uint32_t segmentNumber = 0; segmentNumber != segmentCount; ++segmentNumber) {
		const std::vector<cv::Point>& segment = contour[segmentNumber];
		const uint32_t vertexCount = segment.size();
		contourFile->write((char*)&vertexCount, sizeof(vertexCount)); contourFileOffset += sizeof(vertexCount);
		for (uint32_t vertexNumber = 0; vertexNumber != vertexCount; ++vertexNumber) {
			float x = static_cast<float>(segment[vertexNumber].x);
			float y = static_cast<float>(segment[vertexNumber].y);
			contourFile->write((char*)&x, sizeof(x)); contourFileOffset += sizeof(x);
			contourFile->write((char*)&y, sizeof(y)); contourFileOffset += sizeof(y);
		}
	}
	return currentOffset;
}

std::vector<FlyAttributes>& Arena::getFlyAttributes()
{
	return flyAttributes;
}

void Arena::writeMean(std::ostream& out) const
{
	const char delimiter = '\t';
	std::ostringstream trans;
	frameAttributes.writeMean(trans);
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		flyAttributes[flyNumber].writeMean(trans);
	}
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			pairAttributes[activeFly][passiveFly].writeMean(trans);
		}
	}
//	out << trans.str();
	out << ::transpose(trans.str());
}

// helper functions for Arena::writeBehavior
void Arena::writeFrameBehavior(std::ostream& out, std::string attributeName, size_t frameEnd, size_t framesPerBin, size_t binCount, const char delimiter) const
{
	const Attribute<MyBool>& attribute = frameAttributes.getFilled<MyBool>(attributeName);
	float trueFrames = std::accumulate(attribute.begin(), attribute.begin() + frameEnd, 0.0f);

	out << attributeName << delimiter;
	if (frameEnd > 0) {
		out << (trueFrames / frameEnd);
	}
	out << '\n';

	for (size_t binNumber = 0; binNumber != binCount; ++binNumber) {
		out << attributeName << ", bin " << binNumber << delimiter;
		size_t binBegin = binNumber * framesPerBin;
		size_t binEnd = std::min(binBegin + framesPerBin, frameEnd);
		if (binBegin < binEnd) {
			out << (std::accumulate(attribute.begin() + binBegin, attribute.begin() + binEnd, 0.0f) / (binEnd - binBegin));
		}
		out << '\n';
	}

	std::vector<std::pair<size_t, size_t> > bouts = runs(attribute.begin(), attribute.end());
	out << attributeName << " bouts" << delimiter << bouts.size();
	out << '\n';

	out << attributeName << " bout duration average" << delimiter;
	if (!bouts.empty()) {
		float trueFramesAll = std::accumulate(attribute.begin(), attribute.end(), 0.0f);
		out << (trueFramesAll / bouts.size() / sourceFrameRate);
	}
	out << '\n';

	out << attributeName << " latency" << delimiter;
	const size_t firstFrameTrue = std::find(attribute.begin(), attribute.end(), true) - attribute.begin();
	if (!(firstFrameTrue == attribute.size())) {
		out << (firstFrameTrue / sourceFrameRate);
	}
	out << '\n';
}

void Arena::writeFlyBehavior(std::ostream& out, std::string attributeName, size_t frameEnd, size_t framesPerBin, size_t binCount, const char delimiter) const
{
	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const Attribute<MyBool>& attribute = flyAttributes[flyNumber].getFilled<MyBool>(attributeName);

		out << attributeName << " (" << flyNumber << ")" << delimiter;
		if (frameEnd > 0) {
			out << (std::accumulate(attribute.begin(), attribute.begin() + frameEnd, AttributeTraits<bool>::Mean()) / frameEnd);
		}
		out << '\n';

		for (size_t binNumber = 0; binNumber != binCount; ++binNumber) {
			out << attributeName << " (" << flyNumber << ")" << ", bin " << binNumber << delimiter;
			size_t binBegin = binNumber * framesPerBin;
			size_t binEnd = std::min(binBegin + framesPerBin, frameEnd);
			if (binBegin < binEnd) {
				out << (std::accumulate(attribute.begin() + binBegin, attribute.begin() + binEnd, 0.0f) / (binEnd - binBegin));
			}
			out << '\n';
		}

		std::vector<std::pair<size_t, size_t> > bouts = runs(attribute.begin(), attribute.end());
		out << attributeName << " (" << flyNumber << ")" << " bouts" << delimiter << bouts.size();
		out << '\n';

		out << attributeName << " (" << flyNumber << ")" << " bout duration average" << delimiter;
		if (!bouts.empty()) {
			float trueFramesAll = std::accumulate(attribute.begin(), attribute.end(), 0.0f);
			out << (trueFramesAll / bouts.size() / sourceFrameRate);
		}
		out << '\n';

		out << attributeName << " (" << flyNumber << ")" << " latency" << delimiter;
		const size_t firstFrameTrue = std::find(attribute.begin(), attribute.end(), true) - attribute.begin();
		if (!(firstFrameTrue == attribute.size())) {
			out << (firstFrameTrue / sourceFrameRate);
		}
		out << '\n';
	}
}

void Arena::writePairBehavior(std::ostream& out, std::string attributeName, size_t frameEnd, size_t framesPerBin, size_t binCount, const char delimiter) const
{
	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			const Attribute<MyBool>& attribute = pairAttributes[activeFly][passiveFly].getFilled<MyBool>(attributeName);

			out << attributeName << " (" << activeFly << " -> " << passiveFly << ")" << delimiter;
			if (frameEnd > 0) {
				out << (std::accumulate(attribute.begin(), attribute.begin() + frameEnd, AttributeTraits<bool>::Mean()) / frameEnd);
			}
			out << '\n';

			for (size_t binNumber = 0; binNumber != binCount; ++binNumber) {
				out << attributeName << " (" << activeFly << " -> " << passiveFly << ")" << ", bin " << binNumber << delimiter;
				size_t binBegin = binNumber * framesPerBin;
				size_t binEnd = std::min(binBegin + framesPerBin, frameEnd);
				if (binBegin < binEnd) {
					out << (std::accumulate(attribute.begin() + binBegin, attribute.begin() + binEnd, 0.0f) / (binEnd - binBegin));
				}
				out << '\n';
			}

			std::vector<std::pair<size_t, size_t> > bouts = runs(attribute.begin(), attribute.end());
			out << attributeName << " (" << activeFly << " -> " << passiveFly << ")" << " bouts" << delimiter << bouts.size();
			out << '\n';

			out << attributeName << " (" << activeFly << " -> " << passiveFly << ")" << " bout duration average" << delimiter;
			if (!bouts.empty()) {
				float trueFramesAll = std::accumulate(attribute.begin(), attribute.end(), 0.0f);
				out << (trueFramesAll / bouts.size() / sourceFrameRate);
			}
			out << '\n';

			out << attributeName << " (" << activeFly << " -> " << passiveFly << ")" << " latency" << delimiter;
			const size_t firstFrameTrue = std::find(attribute.begin(), attribute.end(), true) - attribute.begin();
			if (!(firstFrameTrue == attribute.size())) {
				out << (firstFrameTrue / sourceFrameRate);
			}
			out << '\n';
		}
	}
}

void Arena::writeBehavior(std::ostream& out, float binSize, size_t binCount) const
{
	const char delimiter = '\t';
	std::ostringstream trans;

	const size_t framesPerBin = binSize * sourceFrameRate;

	const Attribute<MyBool>& copulating = flyAttributes[0].getFilled<MyBool>("copulating");
	const size_t firstFrameCopulating = std::find(copulating.begin(), copulating.end(), true) - copulating.begin();

	{	// quality
		const Attribute<MyBool>& attribute = frameAttributes.getFilled<MyBool>("isMissegmented");
		float sum = std::accumulate(attribute.begin(), attribute.end(), 0.0f);

		trans << "quality" << delimiter;
		if (attribute.size()) {
			trans << (1.0f - sum / attribute.size());
		}
		trans << '\n';
	}

	{	// copulation
		trans << "copulation" << delimiter;
		if (firstFrameCopulating == copulating.size()) {
			trans << 0;
		} else {
			trans << 1;
		}
		trans << '\n';
	}
	writeFrameBehavior(trans, "courtship", firstFrameCopulating, framesPerBin, binCount);
	writeFlyBehavior(trans, "courting", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "following", firstFrameCopulating, framesPerBin, binCount);
	writeFlyBehavior(trans, "wingExt", firstFrameCopulating, framesPerBin, binCount);
	writeFlyBehavior(trans, "wingExtEitherOr", firstFrameCopulating, framesPerBin, binCount);
	writeFlyBehavior(trans, "wingExtBoth", firstFrameCopulating, framesPerBin, binCount);
	writeFlyBehavior(trans, "wingExtLeft", firstFrameCopulating, framesPerBin, binCount);
	writeFlyBehavior(trans, "wingExtRight", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "wingExtTowards", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "wingExtAway", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "wingExtFront", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "wingExtIpsi", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "wingExtContra", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "wingExtBehind", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "orienting", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "rayEllipseOrienting", firstFrameCopulating, framesPerBin, binCount);
	writePairBehavior(trans, "circling", firstFrameCopulating, framesPerBin, binCount);
	writeFrameBehavior(trans, "isOcclusionTouched", firstFrameCopulating, framesPerBin, binCount);

	{	// speed
		for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
			const Attribute<float>& attribute = flyAttributes[flyNumber].getFilled<float>("movedAbs");
			float sum = std::accumulate(attribute.begin(), attribute.begin() + firstFrameCopulating, 0.0f);

			trans << "movedAbs average [mm/s]" << " (" << flyNumber << ")" << delimiter;
			if (firstFrameCopulating > 0) {
				trans << ((sum / firstFrameCopulating) * (sourceFrameRate / pixelPerMillimeter));
			}
			trans << '\n';
		}
	}

	{	// angular velocity
		for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
			const Attribute<float>& attribute = flyAttributes[flyNumber].getFilled<float>("turnedAbs");
			float sum = std::accumulate(attribute.begin(), attribute.begin() + firstFrameCopulating, 0.0f);

			trans << "turnedAbs average [deg/s]" << " (" << flyNumber << ")" << delimiter;
			if (firstFrameCopulating > 0) {
				trans << ((sum / firstFrameCopulating) * sourceFrameRate);
			}
			trans << '\n';
		}
	}

	{	// mean of the other fly's position during wingExtLeft
		for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
			const Attribute<MyBool>& wingExtLeft = flyAttributes[activeFly].getFilled<MyBool>("wingExtLeft");
			float trueFrames = std::accumulate(wingExtLeft.begin(), wingExtLeft.end(), 0.0f);
			std::vector<std::pair<size_t, size_t> > bouts = runs(wingExtLeft.begin(), wingExtLeft.end());

			for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
				if (activeFly == passiveFly) {
					continue;
				}

				const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
				const Attribute<Vf2>& vectorToOtherLocal = pairAttributes[activeFly][passiveFly].getFilled<Vf2>("vectorToOtherLocal");

				float sumOfDistances = 0;
				Vf2 sumOfDirections;
				for (size_t boutNumber = 0; boutNumber != bouts.size(); ++boutNumber) {
					float sumOfDistancesForThisBout = 0;
					Vf2 sumOfDirectionsForThisBout;
					for (size_t frameNumber = bouts[boutNumber].first; frameNumber != bouts[boutNumber].second; ++frameNumber) {
						sumOfDistancesForThisBout += distanceBodyBody[frameNumber];
						Vf2 direction = vectorToOtherLocal[frameNumber];
						if (direction.norm()) {	// if it's not the 0-vector
							sumOfDirectionsForThisBout += direction.normalize();
						}
					}
					sumOfDistances += sumOfDistancesForThisBout / (bouts[boutNumber].second - bouts[boutNumber].first);
					if (sumOfDirectionsForThisBout.norm()) {	// if it's not the 0-vector
						sumOfDirections += sumOfDirectionsForThisBout.normalize();
					}
				}

				trans << "distance during wingExtLeft" << " (" << activeFly << " -> " << passiveFly << ")" << delimiter;
				if (!bouts.empty()) {
					float meanOfDistances = sumOfDistances / bouts.size();
					trans << (meanOfDistances / pixelPerMillimeter);
				}
				trans << '\n';

				trans << "angle during wingExtLeft" << " (" << activeFly << " -> " << passiveFly << ")" << delimiter;
				if (!bouts.empty()) {
					float angle = std::atan2(sumOfDirections.y(), sumOfDirections.x()) * 180.0 / CV_PI;
					trans << angle;
				}
				trans << '\n';

				trans << "angle similarity during wingExtLeft" << " (" << activeFly << " -> " << passiveFly << ")" << delimiter;
				if (!bouts.empty()) {
					float similarity = sumOfDirections.norm() / bouts.size();
					trans << similarity;
				}
				trans << '\n';
			}
		}
	}

	{	// mean of the other fly's position during wingExtRight
		for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
			const Attribute<MyBool>& wingExtRight = flyAttributes[activeFly].getFilled<MyBool>("wingExtRight");
			float trueFrames = std::accumulate(wingExtRight.begin(), wingExtRight.end(), 0.0f);
			std::vector<std::pair<size_t, size_t> > bouts = runs(wingExtRight.begin(), wingExtRight.end());

			for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
				if (activeFly == passiveFly) {
					continue;
				}

				const Attribute<float>& distanceBodyBody = pairAttributes[activeFly][passiveFly].getFilled<float>("distanceBodyBody");
				const Attribute<Vf2>& vectorToOtherLocal = pairAttributes[activeFly][passiveFly].getFilled<Vf2>("vectorToOtherLocal");

				float sumOfDistances = 0;
				Vf2 sumOfDirections;
				for (size_t boutNumber = 0; boutNumber != bouts.size(); ++boutNumber) {
					float sumOfDistancesForThisBout = 0;
					Vf2 sumOfDirectionsForThisBout;
					for (size_t frameNumber = bouts[boutNumber].first; frameNumber != bouts[boutNumber].second; ++frameNumber) {
						sumOfDistancesForThisBout += distanceBodyBody[frameNumber];
						Vf2 direction = vectorToOtherLocal[frameNumber];
						if (direction.norm()) {	// if it's not the 0-vector
							sumOfDirectionsForThisBout += direction.normalize();
						}
					}
					sumOfDistances += sumOfDistancesForThisBout / (bouts[boutNumber].second - bouts[boutNumber].first);
					if (sumOfDirectionsForThisBout.norm()) {	// if it's not the 0-vector
						sumOfDirections += sumOfDirectionsForThisBout.normalize();
					}
				}

				trans << "distance during wingExtRight" << " (" << activeFly << " -> " << passiveFly << ")" << delimiter;
				if (!bouts.empty()) {
					float meanOfDistances = sumOfDistances / bouts.size();
					trans << (meanOfDistances / pixelPerMillimeter);
				}
				trans << '\n';

				trans << "angle during wingExtRight" << " (" << activeFly << " -> " << passiveFly << ")" << delimiter;
				if (!bouts.empty()) {
					float angle = std::atan2(sumOfDirections.y(), sumOfDirections.x()) * 180.0 / CV_PI;
					trans << angle;
				}
				trans << '\n';

				trans << "angle similarity during wingExtRight" << " (" << activeFly << " -> " << passiveFly << ")" << delimiter;
				if (!bouts.empty()) {
					float similarity = sumOfDirections.norm() / bouts.size();
					trans << similarity;
				}
				trans << '\n';
			}
		}
	}

	out << ::transpose(trans.str());
}

void Arena::paintIntoEthogram(cv::Mat& ethogram, const Attribute<MyBool>& attribute, cv::Vec3b color, size_t paddingLeft, size_t paddingRight, size_t rowBegin, size_t rowEnd) const
{
	assert(ethogram.cols > paddingLeft + paddingRight);
	const Attribute<uint32_t>& videoFrame = frameAttributes.getFilled<uint32_t>("videoFrame");
	const Attribute<float>& videoFrameRelative = frameAttributes.getFilled<float>("videoFrameRelative");
	const size_t widthWithoutPadding = ethogram.cols - paddingLeft - paddingRight;

	// if there was no padding, what would be the first and the last col (inclusive, 0-based)?
	const size_t firstColNoPadding = videoFrameRelative.front() * widthWithoutPadding;
	const size_t lastColNoPadding = videoFrameRelative.back() * widthWithoutPadding;

	const float sampleRatio = (attribute.size() - 1.0f) / (lastColNoPadding - firstColNoPadding);

	for (int col = paddingLeft + firstColNoPadding; col <= paddingLeft + lastColNoPadding; ++col) {
		for (int row = rowBegin; row != rowEnd; ++row) {
			if (attribute[round((col - (paddingLeft + firstColNoPadding)) * sampleRatio)]) {
				ethogram.at<cv::Vec3b>(row, col) = color;
			}
		}
	}
}

void Arena::writeEthograms(const std::string& outDir, const std::string& specification) const
{
	const size_t sliderPadding = 8;	// the part of the slider background that's covered by the slider when it's all the way to the left or right
	const size_t sliderMaxWidth = 4096;	// OSX seems to render these via OpenGL, so we should use a common texture size limit
	assert(sliderMaxWidth > 2 * sliderPadding);
	const size_t sliderHeight = 12;	// should stay at 12, because it can be evenly divided by 2, 3 and 4

	const Attribute<uint32_t>& videoFrame = frameAttributes.getFilled<uint32_t>("videoFrame");
	const Attribute<float>& videoFrameRelative = frameAttributes.getFilled<float>("videoFrameRelative");
	size_t videoFrameCount = videoFrameRelative.size() / (videoFrameRelative.back() - videoFrameRelative.front());	//TODO: this is not numerically stable
	const Attribute<float>& videoTime = frameAttributes.getFilled<float>("videoTime");
	float fps = videoTime.size() / (videoTime.back() - videoTime.front());
	size_t ethoTableCellWidth = (videoFrameCount / fps) * 300.0f / 600.0f;	// 300 pixels for 10 minutes
	const size_t sliderWidthWithPadding = std::min(sliderMaxWidth, videoFrameCount + 2 * sliderPadding);
	const size_t sliderWidthWithoutPadding = sliderWidthWithPadding - 2 * sliderPadding;

	// we can have ethograms for up to 4 flies in the slider background image: its height is 12 pixels, which can be divided by 2, 3 and 4
	const cv::Vec3b white(255, 255, 255);
	cv::Mat ethoSlider(sliderHeight, sliderWidthWithPadding, CV_8UC3, cv::Scalar(120, 120, 120));	// gray background for the entire video
	// white background for the tracked part of the video
	for (int col = sliderPadding + videoFrameRelative.front() * sliderWidthWithoutPadding; col < sliderPadding + sliderWidthWithoutPadding && col < sliderPadding + videoFrameRelative.back() * sliderWidthWithoutPadding; ++col) {
		for (int row = 0; row != ethoSlider.rows; ++row) {
			ethoSlider.at<cv::Vec3b>(row, col) = white;
		}
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		const int fliesInSlider = std::min(static_cast<int>(getFlyCount()), 4);
		const int rowsPerFly = sliderHeight / fliesInSlider;
		const int beginSliderRow = std::min(static_cast<int>(rowsPerFly * flyNumber), 12);
		const int endSliderRow = std::min(static_cast<int>(rowsPerFly * (flyNumber + 1)), 12);
		cv::Mat ethoTableCell(12, ethoTableCellWidth, CV_8UC3, cv::Scalar(120, 120, 120));	// gray background for the entire video

		{	// white background for the tracked part of the video
			size_t sampleRatio = videoFrame.size() / ethoTableCell.cols;
			for (int col = videoFrameRelative.front() * ethoTableCellWidth; col < ethoTableCell.cols && col < videoFrameRelative.back() * ethoTableCellWidth; ++col) {
				for (int row = 0; row != ethoTableCell.rows; ++row) {
					ethoTableCell.at<cv::Vec3b>(row, col) = white;
				}
			}
		}

		{
			const std::vector<std::string> individualSpecifications = split(specification, '|');
			for (int attributeNumber = individualSpecifications.size() - 1; attributeNumber >= 0; --attributeNumber) {	// back-to-front so highest priority attribute is drawn last
				const std::string thisSpecification = individualSpecifications[attributeNumber];
				const int numberOfTokens = 5;	// the number of tokens that have been serialized
				const std::vector<std::string> splitSpecification = split(thisSpecification, ':');

				if (splitSpecification.size() != numberOfTokens) {
					std::cerr << "cannot parse ethogram specification: expected " << numberOfTokens << " tokens, but got " << splitSpecification.size() << " ...skipping" << std::endl;
					continue;
				}

				std::string attributeName = splitSpecification[0];
				std::string attributeKind = splitSpecification[1];

				int red, green, blue;

				try {
					red = unstringify<int>(splitSpecification[2]);
					green = unstringify<int>(splitSpecification[3]);
					blue = unstringify<int>(splitSpecification[4]);
				} catch (std::runtime_error& e) {
					std::cerr << "cannot parse ethogram specification: " << e.what() << " ...skipping" << std::endl;
					continue;
				}

				cv::Vec3b color(blue, green, red);	// remember it's BGR

				if (attributeKind == "frame") {
					if (!frameAttributes.has<MyBool>(attributeName)) {
						std::cerr << "cannot parse ethogram specification: unrecognized frame attribute name: " << attributeName << " ...skipping" << std::endl;
						continue;
					}
					const Attribute<MyBool>& attribute = frameAttributes.getFilled<MyBool>(attributeName);
					paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
					paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
				} else if (attributeKind == "fly") {
					if (!flyAttributes[flyNumber].has<MyBool>(attributeName)) {
						std::cerr << "cannot parse ethogram specification: unrecognized fly attribute name: " << attributeName << " ...skipping" << std::endl;
						continue;
					}
					const Attribute<MyBool>& attribute = flyAttributes[flyNumber].getFilled<MyBool>(attributeName);
					paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
					paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
				} else if (attributeKind == "pair") {
					for (size_t passiveFlyNumber = 0; passiveFlyNumber != getFlyCount(); ++passiveFlyNumber) {
						if (passiveFlyNumber == flyNumber) {
							continue;
						}

						if (!pairAttributes[flyNumber][passiveFlyNumber].has<MyBool>(attributeName)) {
							std::cerr << "cannot parse ethogram specification: unrecognized pair attribute name: " << attributeName << " ...skipping" << std::endl;
							continue;
						}
						const Attribute<MyBool>& attribute = pairAttributes[flyNumber][passiveFlyNumber].getFilled<MyBool>(attributeName);
						paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
						paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
					}
				} else {
					std::cerr << "cannot parse ethogram specification: unrecognized attribute kind: " << attributeKind << " ...skipping" << std::endl;
					continue;
				}
			}
		}
/*
		{	// orienting
			cv::Vec3b color(255, 124, 81);	// remember it's BGR
			for (size_t passiveFlyNumber = 0; passiveFlyNumber != getFlyCount(); ++passiveFlyNumber) {
				if (passiveFlyNumber == flyNumber) {
					continue;
				}
				const Attribute<MyBool>& attribute = pairAttributes[flyNumber][passiveFlyNumber].getFilled<MyBool>("orienting");
				paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
				paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
			}
		}

		{	// following
			cv::Vec3b color(0, 255, 0);	// remember it's BGR
			for (size_t passiveFlyNumber = 0; passiveFlyNumber != getFlyCount(); ++passiveFlyNumber) {
				if (passiveFlyNumber == flyNumber) {
					continue;
				}
				const Attribute<MyBool>& attribute = pairAttributes[flyNumber][passiveFlyNumber].getFilled<MyBool>("following");
				paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
				paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
			}
		}

		{	// wingExt
			cv::Vec3b color(0, 0, 255);	// remember it's BGR
			const Attribute<MyBool>& attribute = flyAttributes[flyNumber].getFilled<MyBool>("wingExt");
			paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
			paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
		}

		{	// circling
			cv::Vec3b color(255, 81, 200);	// remember it's BGR
			for (size_t passiveFlyNumber = 0; passiveFlyNumber != getFlyCount(); ++passiveFlyNumber) {
				if (passiveFlyNumber == flyNumber) {
					continue;
				}
				const Attribute<MyBool>& attribute = pairAttributes[flyNumber][passiveFlyNumber].getFilled<MyBool>("circling");
				paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
				paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
			}
		}

		{	// copulating
			cv::Vec3b color(0, 255, 255);	// remember it's BGR
			const Attribute<MyBool>& attribute = flyAttributes[flyNumber].getFilled<MyBool>("copulating");
			paintIntoEthogram(ethoTableCell, attribute, color, 0, 0, 0, ethoTableCell.rows);
			paintIntoEthogram(ethoSlider, attribute, color, sliderPadding, sliderPadding, beginSliderRow, endSliderRow);
		}
*/
		imwrite(outDir + "/" + getId() + "/" + stringify(flyNumber) + "_ethoTableCell.png", ethoTableCell);
	}

	imwrite(outDir + "/" + getId() + "/ethoSlider.png", ethoSlider);
}

void Arena::writeMissegmented(std::ostream& out) const
{
	if (getFlyCount() != 2) {	//TODO: make this work for an arbitrary number of flies
		return;
	}

	const Attribute<MyBool>& isMissegmentedUnmerged = frameAttributes.getFilled<MyBool>("isMissegmentedUnmerged");
	//TODO: make this work for an arbitrary number of flies
	const Attribute<Vf2>& fly0Centroid = flyAttributes[0].getFilled<Vf2>("bodyCentroid");
	const Attribute<Vf2>& fly1Centroid = flyAttributes[1].getFilled<Vf2>("bodyCentroid");

	const char delimiter = '\t';

	bool previousFrameMissegmented = false;
	size_t currentBegin = 0;
	for (size_t frameNumber = 0; frameNumber != isMissegmentedUnmerged.size(); ++frameNumber) {
		if (isMissegmentedUnmerged[frameNumber]) {
			if (!previousFrameMissegmented) {
				currentBegin = frameNumber;
			}
			previousFrameMissegmented = true;
		} else { // non-missegmented frame
			if (previousFrameMissegmented && currentBegin != 0) {
				// write a pos score
				size_t beforeMissegmentation = currentBegin - 1;
				size_t afterMissegmentation = frameNumber;
				float firstToFirst = (fly0Centroid[beforeMissegmentation] - fly0Centroid[afterMissegmentation]).norm();
				float firstToSecond = (fly0Centroid[beforeMissegmentation] - fly1Centroid[afterMissegmentation]).norm();
				float secondToFirst = (fly1Centroid[beforeMissegmentation] - fly0Centroid[afterMissegmentation]).norm();
				float secondToSecond = (fly1Centroid[beforeMissegmentation] - fly1Centroid[afterMissegmentation]).norm();
				float straightDistance = firstToFirst + secondToSecond;
				float swappedDistance = firstToSecond + secondToFirst;
				float score = (swappedDistance - straightDistance) / (straightDistance + swappedDistance);

				out <<
					currentBegin << delimiter <<
					frameNumber << delimiter <<
					frameNumber - currentBegin << delimiter <<
					score << '\n';
			}
			previousFrameMissegmented = false;
		}
	}
	// if it's ending with a missegmentation
	if (previousFrameMissegmented) {
		// we can't get a pos score if there's no end frame, so do nothing
	}
}

void Arena::writePositionCorrelation(std::ostream& out) const
{
	if (getFlyCount() != 2) {	//TODO: make this work for an arbitrary number of flies
		return;
	}

	const char delimiter = '\t';

	const Attribute<MyBool>& isMissegmentedUnmerged = frameAttributes.getFilled<MyBool>("isMissegmentedUnmerged");
	const Attribute<MyBool>& isOcclusion = frameAttributes.getFilled<MyBool>("isOcclusion");
	//TODO: make this work for an arbitrary number of flies
	const Attribute<Vf2>& fly0Centroid = flyAttributes[0].getFilled<Vf2>("bodyCentroid");
	const Attribute<Vf2>& fly1Centroid = flyAttributes[1].getFilled<Vf2>("bodyCentroid");

	const size_t frameCount = isMissegmentedUnmerged.size();
	const size_t trialCount = 100;
	const size_t maxFrameDelta = 11;

	for (size_t trial = 0; trial != trialCount; ++trial) {
		size_t frameStart = static_cast<size_t>(static_cast<float>(rand()) / RAND_MAX * frameCount);
		if (frameStart + maxFrameDelta >= frameCount) {
			continue;
		}
		for (size_t frameDelta = 0; frameDelta <= maxFrameDelta; ++frameDelta) {
			if (isMissegmentedUnmerged[frameStart + frameDelta] || isOcclusion[frameStart + frameDelta]) {
				goto NEXT_TRIAL;
			}
		}

		out << frameStart;

		for (size_t frameDelta = 1; frameDelta <= maxFrameDelta; ++frameDelta) {
			// write a pos score
			size_t frameStop = frameStart + frameDelta;
			float firstToFirst = (fly0Centroid[frameStart] - fly0Centroid[frameStop]).norm();
			float firstToSecond = (fly0Centroid[frameStart] - fly1Centroid[frameStop]).norm();
			float secondToFirst = (fly1Centroid[frameStart] - fly0Centroid[frameStop]).norm();
			float secondToSecond = (fly1Centroid[frameStart] - fly1Centroid[frameStop]).norm();

			float straightDistance = firstToFirst + secondToSecond;
			float swappedDistance = firstToSecond + secondToFirst;

			float score = (swappedDistance - straightDistance) / (straightDistance + swappedDistance);

			out << delimiter << score;
		}

		out << '\n';

		NEXT_TRIAL: ;
	}
}
