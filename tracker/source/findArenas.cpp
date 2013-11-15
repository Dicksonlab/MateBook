#include "findArenas.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <functional>
#include "findCircles.hpp"
#include "statistics.hpp"

class ArenaComparator : public std::binary_function<const cv::Vec3f&, const cv::Vec3f&, bool> {
public:
	ArenaComparator(size_t videoWidth, size_t videoHeight) :
		videoWidth(videoWidth),
		videoHeight(videoHeight)
	{
	}

	bool operator()(const cv::Vec3f& first, const cv::Vec3f& second) const
	{
		float firstX = first[0];
		float firstY = first[1];
		float firstR = first[2];
		// skew operation moving y down for "firstR" at most
		float firstCompY = firstY + firstR * firstX / videoWidth;

		float secondX = second[0];
		float secondY = second[1];
		float secondR = second[2];
		// skew operation moving y down for "secondR" at most
		float secondCompY = secondY + secondR * secondX / videoWidth;

		return firstCompY < secondCompY;
	}

private:
	size_t videoWidth;
	size_t videoHeight;
};

std::vector<Arena> findArenas(const cv::Mat& image, Shape shape, double sourceFrameRate, size_t flyCount, float diameter, float borderSize, Interior interior)
{
	if (shape == CIRCLE || shape == RING) {
		std::vector<cv::Vec3f> circles = findCircles(image, interior);
		std::sort(circles.begin(), circles.end(), ArenaComparator(image.cols, image.rows));

		// build the vector of arenas
		std::vector<Arena> arenas;
		unsigned int arenaId = 0;
		for (std::vector<cv::Vec3f>::const_iterator iter = circles.begin(); iter != circles.end(); ++iter) {
			cv::Point center(cvRound((*iter)[0]), cvRound((*iter)[1]));
			// increase the radius by half the given arena border size; note that this affects how pixelPerMillimeter is calculated in the Arena constructor
			int radius = cvRound((*iter)[2] * (1 + borderSize / diameter));
			cv::Rect boundingBox(center.x - radius, center.y - radius, 2 * radius, 2 * radius);
			if (boundingBox.x < 0 || boundingBox.x + boundingBox.width > image.cols ||
				boundingBox.y < 0 || boundingBox.y + boundingBox.height > image.rows) {
				// skip circles that cross the video border
				continue;
			}

			// draw the arena mask
			cv::Mat arenaMask = drawArenaMask(boundingBox.size(), shape, borderSize);

			arenas.push_back(Arena("DetectedArena_" + stringify(++arenaId), sourceFrameRate, flyCount, boundingBox, diameter, borderSize, arenaMask, cv::Mat(image, boundingBox)));
			//TODO: clamp to frame size in case of rounding errors
		}
		return arenas;
	} else if (shape == RECTANGLE) {
		cv::Mat bgMedianGray;
		cvtColor(image, bgMedianGray, CV_BGR2GRAY);
		double bgMean = ::mean<unsigned char>(bgMedianGray);
		double bgStddev = ::stddev<unsigned char>(bgMedianGray);
		unsigned char thresh = bgMean;
		cv::Mat bwArenas;
		threshold(bgMedianGray, bwArenas, thresh, 255, cv::THRESH_BINARY);

		std::vector<Arena> arenas;
		unsigned int arenaId = 0;

		// for each row check if more than half the pixels are darker than the mean to see if it's an arena
		size_t arenaRows = 0;
		for (size_t rowIndex = 0; rowIndex != bwArenas.rows; ++rowIndex) {
			cv::Mat row = bwArenas.row(rowIndex);
			size_t darkCount = 0;
			for (size_t colIndex = 0; colIndex != row.cols; ++colIndex) {
				if (*(row.data + colIndex) == 0) {
					++darkCount;
				}
			}
			if (darkCount >= bwArenas.cols / 2) {
				++arenaRows;
			} else {
				if (arenaRows > 10) { // sanity check: arenas must have at least 10 rows
					int rowIndexFirst = rowIndex - arenaRows;
					int rowIndexLast = rowIndex - 1;
					arenaRows = rowIndexLast - rowIndexFirst + 1;

					// for the left and right borders, check which column-wise means are below the mean in the area
					int borderArea = bgMedianGray.cols / 10;
					double leftMean = ::mean<unsigned char>(cv::Mat(bgMedianGray, cv::Rect(0, rowIndexFirst, borderArea, arenaRows)));
					int colIndexFirst = 0;
					while (colIndexFirst < bgMedianGray.cols) {
						if (::mean<unsigned char>(cv::Mat(bgMedianGray, cv::Rect(colIndexFirst, rowIndexFirst, 1, arenaRows))) < leftMean) {
							break;
						}
						++colIndexFirst;
					}
					double rightMean = ::mean<unsigned char>(cv::Mat(bgMedianGray, cv::Rect(bgMedianGray.cols - borderArea, rowIndexFirst, borderArea, arenaRows)));
					int colIndexLast = bgMedianGray.cols - 1;
					while (colIndexLast >= 0) {
						if (::mean<unsigned char>(cv::Mat(bgMedianGray, cv::Rect(colIndexLast, rowIndexFirst, 1, arenaRows))) < rightMean) {
							break;
						}
						--colIndexLast;
					}
					if (colIndexFirst >= colIndexLast) {
						std::cout << "failed to detect the left and right arena borders, using entire frame" << std::endl;
						colIndexFirst = 0;
						colIndexLast = bgMedianGray.cols - 1;
					}
					size_t arenaCols = colIndexLast - colIndexFirst + 1;

					// add another 5 rows before and after
					rowIndexFirst = cv::max(0, rowIndexFirst - 5);
					rowIndexLast = cv::min(bwArenas.rows - 1, rowIndexLast + 5);
					arenaRows = rowIndexLast - rowIndexFirst + 1;
					
					// add another 5 cols before and after
					colIndexFirst = cv::max(0, colIndexFirst - 5);
					colIndexLast = cv::min(bwArenas.cols - 1, colIndexLast + 5);
					arenaCols = colIndexLast - colIndexFirst + 1;

					cv::Rect boundingBox(colIndexFirst, rowIndexFirst, arenaCols, arenaRows);
					cv::Mat mask = drawArenaMask(cv::Size(arenaCols, arenaRows), shape, 0);
					arenas.push_back(Arena("DetectedArena_" + stringify(++arenaId), sourceFrameRate, flyCount, boundingBox, diameter, borderSize, mask, cv::Mat(image, boundingBox).clone()));
				}
				arenaRows = 0;
			}
		}
		std::cout << "info: found " << arenas.size() << " arenas" << std::endl;
		return arenas;
	} else {
		throw std::runtime_error(stringify(shape) + " is not a supported arena shape");
	}
}

cv::Mat drawArenaMask(cv::Size size, Shape shape, float borderSize)
{
	if (shape == CIRCLE || shape == RING) {
		cv::Mat arenaMask(size, CV_8UC1, cv::Scalar(0));
		int radius = size.width / 2;
		circle(arenaMask, cv::Point(size.width / 2, size.height / 2), radius, cv::Scalar(255), -1, 8, 0);
		if (shape == RING) {
			circle(arenaMask, cv::Point(size.width / 2, size.height / 2), radius / 1.3, cv::Scalar(0), -1, 8, 0);
		}

		//TODO: remove this workaround once the contour tracing is fixed
		// here we're setting the first and last columns and rows to false because OpenCV's contour tracing is flawed and doesn't pick up those pixels
		// this would then lead to problems in Arena::track()
		if (arenaMask.rows > 0 && arenaMask.cols > 0) {
			for (int row = 0; row != arenaMask.rows; ++row) {
				arenaMask.at<bool>(row, 0) = false;
				arenaMask.at<bool>(row, arenaMask.cols - 1) = false;
			}
			for (int col = 0; col != arenaMask.cols; ++col) {
				arenaMask.at<bool>(0, col) = false;
				arenaMask.at<bool>(arenaMask.rows - 1, col) = false;
			}
		}

		return arenaMask;
	} else if (shape == RECTANGLE) {
		cv::Mat arenaMask(size, CV_8UC1, cv::Scalar(255));
		return arenaMask;
	} else {
		throw std::runtime_error(stringify(shape) + " is not a supported arena shape");
	}
}
