#include "findCircles.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <stdexcept>
#include <vector>
#include <iostream>
#include <limits>

#include "global.hpp"
#include "../../common/source/mathematics.hpp"
#include "../../common/source/serialization.hpp"

int wrap(int index, int containerSize)
{
	index = index % containerSize;
	if (index < 0) {
		index += containerSize;
	}
	return index;
}

// writes a normalized version of an image to a file and returns the normalization factor used
template<class T>
double imwriteNorm(const std::string& filePath, const cv::Mat_<T>& image)
{
	double imageMax;
	cv::minMaxLoc(image, NULL, &imageMax);
	double normalizationFactor = 255.0 / imageMax;
	imwrite(filePath, image * normalizationFactor);
	return normalizationFactor;
}

class Circle {
public:
	Circle(float x, float y, size_t radiusBufferSize) :
		x(x),
		y(y),
		radiusBuffer(radiusBufferSize),
		radiusBufferEntropy(),
		radiusBufferMean(),
		radiusBufferStddev(),
		radiusAngleBufferMinMax()
	{
	}

	void calculateStatistics()
	{
		radiusBufferEntropy = entropy(radiusBuffer);
		radiusBufferMean = mean(radiusBuffer);
		radiusBufferStddev = stddev(radiusBuffer);
	}

	void calculateRadiusAngleBufferMinMax()
	{
		cv::Mat_<float> maxPerColumn;
		cv::reduce(radiusAngleBuffer, maxPerColumn, 0, CV_REDUCE_MAX);
		cv::Mat_<float> minOfMaxPerColumn;
		cv::reduce(maxPerColumn, minOfMaxPerColumn, 1, CV_REDUCE_MIN);
		radiusAngleBufferMinMax = minOfMaxPerColumn(0, 0);
	}

	float x;
	float y;
	std::vector<float> radiusBuffer;
	float radiusBufferEntropy;
	float radiusBufferMean;
	float radiusBufferStddev;
	cv::Mat_<float> radiusAngleBuffer;	// access with (r - minRadiusAllowed, phi) with phi binned into 40
	float radiusAngleBufferMinMax;	// the lowest maximum gradient for any angle
};

class increasingEntropy {
public:
	bool operator()(const Circle& left, const Circle& right)
	{
		return left.radiusBufferEntropy < right.radiusBufferEntropy;
	}
};

class decreasingRadiusAngleBufferMinMax {
public:
	bool operator()(const Circle& left, const Circle& right)
	{
		return left.radiusAngleBufferMinMax > right.radiusAngleBufferMinMax;
	}
};

cv::Mat_<float> circleCenters(const cv::Mat_<float>& gradientX, const cv::Mat_<float>& gradientY, const cv::Mat_<float>& gradientMagnitude, int minRadius, int maxRadius, Interior interior)
{
	// accumulate votes for circle centers by walking along the direction of the gradients
    const int SHIFT = 10, ONE = 1 << SHIFT;
	cv::Mat_<float> centerBuffer(gradientX.size(), 0.0f);
	for (int y = 0; y < gradientX.rows; ++y) {
		for (int x = 0; x < gradientX.cols; ++x) {
			float vx = gradientX(y, x);
			float vy = gradientY(y, x);
			
			if (vx == 0 && vy == 0) {
				continue;
			}
			
			float mag = gradientMagnitude(y, x);
			assert(mag >= 1);
			int sx = cvRound((vx)*ONE/mag);
			int sy = cvRound((vy)*ONE/mag);
			
			int x0 = cvRound((x)*ONE);
			int y0 = cvRound((y)*ONE);
			
			if (interior == BRIGHT || interior == EITHER) {
			    int x1 = x0 + minRadius * sx;
			    int y1 = y0 + minRadius * sy;
				
			    for (int r = minRadius; r <= maxRadius; x1 += sx, y1 += sy, r++) {
			        int x2 = x1 >> SHIFT, y2 = y1 >> SHIFT;
			        if((unsigned)x2 >= (unsigned)gradientX.cols || (unsigned)y2 >= (unsigned)gradientX.rows) {
			            break;
					}
					int xdist = x2 - x;
					int ydist = y2 - y;
					float radius = sqrt(static_cast<float>(xdist * xdist + ydist * ydist));
					centerBuffer(y2, x2) += mag / radius;
			    }
			}
			if (interior == DARK || interior == EITHER) {
			    sx = -sx;
				sy = -sy;

			    int x1 = x0 + minRadius * sx;
			    int y1 = y0 + minRadius * sy;
				
			    for (int r = minRadius; r <= maxRadius; x1 += sx, y1 += sy, r++) {
			        int x2 = x1 >> SHIFT, y2 = y1 >> SHIFT;
			        if((unsigned)x2 >= (unsigned)gradientX.cols || (unsigned)y2 >= (unsigned)gradientX.rows) {
			            break;
					}
					int xdist = x2 - x;
					int ydist = y2 - y;
					float radius = sqrt(static_cast<float>(xdist * xdist + ydist * ydist));
					centerBuffer(y2, x2) += mag / radius;
			    }
			}
        }
    }
	return centerBuffer;
}

std::vector<cv::Vec3f> findCircles(const cv::Mat& image, Interior interior)
{
	const int minRadius = 80;
	const int maxRadius = std::min(image.rows, image.cols) / 2;
	const size_t maxRadiusVoters = 10;
	const size_t angleBinCount = 40;
	
	cv::Mat bgMedianGray;
	cvtColor(image, bgMedianGray, CV_BGR2GRAY);
	GaussianBlur(bgMedianGray, bgMedianGray, cv::Size(7, 7), 1.5, 1.5);

	// create and save gradient images
    cv::Mat_<float> gradientX;
	cv::Sobel(bgMedianGray, gradientX, gradientX.depth(), 1, 0, 3);
	imwrite(global::outDir + "/" + "gradientX.png", gradientX);

	cv::Mat_<float> gradientY;
	cv::Sobel(bgMedianGray, gradientY, gradientY.depth(), 0, 1, 3);
	imwrite(global::outDir + "/" + "gradientY.png", gradientY);

	cv::Mat_<float> gradientMagnitude = gradientX.mul(gradientX) + gradientY.mul(gradientY);
	cv::sqrt(gradientMagnitude, gradientMagnitude);
	imwrite(global::outDir + "/" + "gradientMagnitude.png", gradientMagnitude);

	// get the center accumulation buffer and save it as an image
	cv::Mat_<float> centerBuffer = circleCenters(gradientX, gradientY, gradientMagnitude, minRadius, maxRadius, interior);
	std::cout << "centerBuffer.png normalized with factor " << imwriteNorm(global::outDir + "/" + "centerBuffer.png", centerBuffer) << std::endl;

	// preselect local maxima in a blurred accumulation buffer as potential centers
	cv::Mat_<float> blurredCenterBuffer;
	cv::GaussianBlur(centerBuffer, blurredCenterBuffer, cv::Size(roundToOdd(minRadius), roundToOdd(minRadius)), minRadius / 6.0, minRadius / 6.0);
	std::vector<Circle> candidates;
	cv::Mat_<float> dilatedCenterBuffer;
	cv::dilate(blurredCenterBuffer, dilatedCenterBuffer, cv::Mat());
	for (size_t y = 0; y < centerBuffer.rows; ++y) {
		for (size_t x = 0; x < image.cols; ++x) {
			if (blurredCenterBuffer(y, x) == dilatedCenterBuffer(y, x)) {
				// local maximum since dilation didn't change the pixel value
				if (std::min(x, image.cols - x) > minRadius && std::min(y, image.rows - y) > minRadius) {
					// not too close to the border
					candidates.push_back(Circle(x, y, maxRadius));	//TODO: we shouldn't construct with maxRadius
				}
			}
		}
	}
	std::cout << "found " << candidates.size() << " local maxima" << std::endl;

	// build local radius accumulation buffers
	for (size_t candidateNumber = 0; candidateNumber != candidates.size(); ++candidateNumber) {
		for (int y = candidates[candidateNumber].y - maxRadius; y < candidates[candidateNumber].y + maxRadius; ++y) {
			for (int x = candidates[candidateNumber].x - maxRadius; x < candidates[candidateNumber].x + maxRadius; ++x) {
				int xDist = abs(x - candidates[candidateNumber].x);
				int yDist = abs(y - candidates[candidateNumber].y);
				float radius = sqrt(static_cast<float>(xDist * xDist + yDist * yDist));
				int iRadius = round(radius);
				// restrict to circle
				if (iRadius == 0 || iRadius >= candidates[candidateNumber].radiusBuffer.size()) {
					continue;
				}
				// we wrap around so all candidates have the same size radius buffer
				int xIndex = wrap(x, image.cols);
				int yIndex = wrap(y, image.rows);
				float contribution = 0;
				if (interior == BRIGHT) {
					float vx = gradientX(yIndex, xIndex);
					float vy = gradientY(yIndex, xIndex);

					float normalization = std::sqrt((xDist * xDist + yDist * yDist) * (vx * vx + vy * vy));
					if (normalization != 0) {
						float cosOfDifference = (xDist * vx + yDist * vy) / normalization;
						contribution = std::max(-cosOfDifference, 0.0f) / radius;
					}
				} else if (interior == DARK) {
					float vx = gradientX(yIndex, xIndex);
					float vy = gradientY(yIndex, xIndex);

					float normalization = std::sqrt((xDist * xDist + yDist * yDist) * (vx * vx + vy * vy));
					if (normalization != 0) {
						float cosOfDifference = (xDist * vx + yDist * vy) / normalization;
						contribution = std::max(cosOfDifference, 0.0f) / radius;
					}
				} else {
					contribution = gradientMagnitude(yIndex, xIndex) / radius;
				}
				candidates[candidateNumber].radiusBuffer[iRadius] += contribution;
			}
		}
		candidates[candidateNumber].calculateStatistics();
	}

	std::sort(candidates.begin(), candidates.end(), increasingEntropy());

	// visualize the blurred buffer and enumerate the maxima in the image
	cv::Mat_<float> blurredImage = blurredCenterBuffer.clone();
	for (size_t candidateNumber = 0; candidateNumber != candidates.size(); ++candidateNumber) {
		cv::putText(
			blurredImage,
			stringify(candidateNumber),
			cv::Point(candidates[candidateNumber].x, candidates[candidateNumber].y),
			CV_FONT_HERSHEY_PLAIN,
			1,
			0
		);
	}
	imwriteNorm(global::outDir + "/" + "blurredCenterBuffer.png", blurredImage);

	// build global radius accumulation buffer by summing the buffers of the candidates with the lowest entropy
	std::vector<float> radiusBuffer(maxRadius);
	for (size_t candidateNumber = 0; candidateNumber != std::min(maxRadiusVoters, candidates.size()); ++candidateNumber) {
		for (size_t radius = 0; radius != maxRadius; ++ radius) {
			radiusBuffer[radius] += candidates[candidateNumber].radiusBuffer[radius];
		}
	}

	// the index of the maximum in the global buffer denotes the radius the candidates agree on
	std::vector<float>::const_iterator maxIter = max_element(radiusBuffer.begin() + minRadius, radiusBuffer.end());
	size_t maxIndex = maxIter - radiusBuffer.begin();
	float maxCount = *maxIter;
	float globalRadius = maxIndex;
	float minRadiusAllowed = globalRadius - globalRadius / 100.0f * 5.0f;
	float maxRadiusAllowed = globalRadius + globalRadius / 100.0f * 5.0f;
	std::cout << "radius is " << globalRadius << " (" << minRadiusAllowed << " ... " << maxRadiusAllowed << ")" << std::endl;

	// accumulate votes for circle centers again, but this time with a more restricted radius range
	cv::Mat_<float> restrictedCenterBuffer = circleCenters(gradientX, gradientY, gradientMagnitude, minRadiusAllowed, maxRadiusAllowed, interior);

	// save the center accumulation buffer as an image
	std::cout << "centerBuffer2.png normalized with factor " << imwriteNorm(global::outDir + "/" + "centerBuffer2.png", restrictedCenterBuffer) << std::endl;

	// preselect local maxima in a blurred accumulation buffer as potential centers
	cv::Mat_<float> blurredRestrictedAccumulationBuffer;
	cv::GaussianBlur(restrictedCenterBuffer, blurredRestrictedAccumulationBuffer, cv::Size(roundToOdd(minRadius), roundToOdd(minRadius)), minRadius / 6.0, minRadius / 6.0);
	std::vector<Circle> restrictedCandidates;
	cv::Mat_<float> dilatedRestrictedAccumulationBuffer;
	cv::dilate(blurredRestrictedAccumulationBuffer, dilatedRestrictedAccumulationBuffer, cv::Mat());
	for (size_t y = 0; y < restrictedCenterBuffer.rows; ++y) {
		for (size_t x = 0; x < image.cols; ++x) {
			if (blurredRestrictedAccumulationBuffer(y, x) == dilatedRestrictedAccumulationBuffer(y, x)) {
				// local maximum since dilation didn't change the pixel value
				if (std::min(x, image.cols - x) > minRadiusAllowed && std::min(y, image.rows - y) > minRadiusAllowed) {
					// not too close to the border
					restrictedCandidates.push_back(Circle(x, y, maxRadius));	//TODO: we shouldn't construct with maxRadius
				}
			}
		}
	}
	std::cout << "found " << restrictedCandidates.size() << " local maxima" << std::endl;

	// visualize the blurred accumulation buffer and enumerate the maxima in the image
	cv::Mat_<float> blurredRestrictedImage = blurredRestrictedAccumulationBuffer.clone();
	for (size_t candidateNumber = 0; candidateNumber != restrictedCandidates.size(); ++candidateNumber) {
		cv::putText(
			blurredRestrictedImage,
			stringify(candidateNumber),
			cv::Point(restrictedCandidates[candidateNumber].x, restrictedCandidates[candidateNumber].y),
			CV_FONT_HERSHEY_PLAIN,
			1,
			0
		);
	}
	imwriteNorm(global::outDir + "/" + "blurredCenterBuffer2.png", blurredRestrictedImage);

	// calculate gradient statistics (which are currently not being used)
	const float meanGradientMagnitude = mean(gradientMagnitude.begin(), gradientMagnitude.end());
	const float stdGradientMagnitude = stddev(gradientMagnitude.begin(), gradientMagnitude.end());
	std::cout << "gradient magnitude mean is " << meanGradientMagnitude << ", stddev is " << stdGradientMagnitude << std::endl;

	// check which of the potential circles have large gradients for every angle
	int iMinRadiusAllowed = (int)minRadiusAllowed;
	int iMaxRadiusAllowed = (int)maxRadiusAllowed;
	for (size_t i = 0; i < restrictedCandidates.size(); ++i) {
		// draw the radius buffer
		restrictedCandidates[i].radiusAngleBuffer = cv::Mat_<float>(iMaxRadiusAllowed - iMinRadiusAllowed, angleBinCount, 0.0f);

		for (int y = 0; y < image.rows; ++y) {
			int dY = y - restrictedCandidates[i].y;
			for (int x = 0; x < image.cols; ++x) {
				int dX = x - restrictedCandidates[i].x;
				float distance = sqrt(static_cast<float>(dX * dX + dY * dY));
				int iDistance = round(distance);
				if (((iDistance) < iMinRadiusAllowed) || (iDistance >= iMaxRadiusAllowed)) {
					continue;
				}
				float angle = std::atan2((float)dY, (float)dX);
				int angleBin = static_cast<int>((CV_PI + angle) * angleBinCount / (2 * CV_PI));
				if (angleBin == angleBinCount) {	// wrap around for pixels directly to the left
					angleBin = 0;
				}
				assert(angleBin >= 0 && angleBin < angleBinCount);

				float vx = gradientX(y, x);
				float vy = gradientY(y, x);
				float mag = std::sqrt(vx * vx + vy * vy);

				restrictedCandidates[i].radiusAngleBuffer(iDistance - iMinRadiusAllowed, angleBin) += mag;
			}
		}
		//imwriteNorm(global::outDir + "/" + "radiusAngle_" + stringify(i) + ".png", restrictedCandidates[i].radiusAngleBuffer);
		restrictedCandidates[i].calculateRadiusAngleBufferMinMax();
	}

	// accept the best circles until we find one that overlaps one of the accepted ones
	std::sort(restrictedCandidates.begin(), restrictedCandidates.end(), decreasingRadiusAngleBufferMinMax());
	std::vector<cv::Vec3f> circles;
	for (size_t i = 0; i < restrictedCandidates.size(); ++i) {
		bool rejected = false;
		for (std::vector<cv::Vec3f>::const_iterator accepted = circles.begin(); accepted != circles.end(); ++accepted) {
			if (((*accepted)[0] - restrictedCandidates[i].x) * ((*accepted)[0] - restrictedCandidates[i].x) +
				((*accepted)[1] - restrictedCandidates[i].y) * ((*accepted)[1] - restrictedCandidates[i].y) <
				4 * globalRadius * globalRadius) {
				rejected = true;
				break;
			}
		}
		if (rejected) {
			break;
		}
		circles.push_back(cv::Vec3f(restrictedCandidates[i].x, restrictedCandidates[i].y, globalRadius));
	}

	return circles;
}
