#include "getBodyThreshold.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>

unsigned char getBodyThreshold(cv::Mat foreground, boost::shared_ptr<std::ofstream> smoothHistogramFile)
{
	int hist[256] = {};
	for (cv::MatIterator_<uchar> iter = foreground.begin<uchar>(); iter != foreground.end<uchar>(); ++iter) {
		++hist[*iter];
	}
	// moving average filter, clipping the kernel at the borders
	// (note that moving average filters can be implemented more efficiently by adjusting the old average using only the border values when moving to the next element)
	int windowLength = 25;
	int halfWindowLength = windowLength / 2;
	float smoothedHist[256] = {};
	for (int i = 0; i != 256; ++i) {
		int beginJ = cv::max(0, i - halfWindowLength);
		int endJ = cv::min(256, i + halfWindowLength + 1);
		float sum = 0;
		for (int j = beginJ; j != endJ; ++j) {
			sum += hist[j];
		}
		smoothedHist[i] = sum / (endJ - beginJ);
	}

	if (smoothHistogramFile) {
		smoothHistogramFile->write((char*)&smoothedHist[0], 256 * sizeof(float));
	}

	// find the first gray value where counts are increasing (after skipping the first 25 + 30, the latter being the typical difference between interior and exterior background)
	int slopeStart = 55;
	for (int i = slopeStart; i != 255; ++i) {
		if (smoothedHist[i + 1] > smoothedHist[i]) {
			slopeStart = i;
			break;
		}
	}
	// find maximum after slopeStart
	float* maxElementPointer = std::max_element(smoothedHist + slopeStart, smoothedHist + 256);
	float* threshPointer = std::min_element(smoothedHist + slopeStart, maxElementPointer);
	unsigned char thresh = static_cast<unsigned char>(threshPointer - smoothedHist);
	return thresh;
}
