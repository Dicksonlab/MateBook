#include "inpaint.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>

cv::Mat inpaint(const cv::Mat& image, const cv::Mat& mask)
{
	//TODO: implement http://sci.tech-archive.net/Archive/sci.image.processing/2009-04/msg00045.html
	cv::Mat ret;
//	cv::inpaint(image, mask, ret, 2, cv::INPAINT_TELEA);
//	cv::inpaint(image, mask, ret, 2, cv::INPAINT_NS);

	// inpaint with the median color
	std::vector<unsigned char> red;
	std::vector<unsigned char> green;
	std::vector<unsigned char> blue;
	ret = image.clone();
	for (int row = 0; row != image.rows; ++row) {
		for (int col = 0; col != image.cols; ++col) {
			if (mask.at<unsigned char>(row, col)) {
				cv::Vec3b bgr = image.at<cv::Vec3b>(row, col);
				red.push_back(bgr[2]);
				green.push_back(bgr[1]);
				blue.push_back(bgr[0]);
			}
		}
	}
	if (!red.empty()) {
		std::vector<unsigned char>::iterator medianIter = red.begin() + red.size() / 2;
		std::nth_element(red.begin(), medianIter, red.end());
		unsigned char medianRed = *medianIter;

		medianIter = green.begin() + green.size() / 2;
		std::nth_element(green.begin(), medianIter, green.end());
		unsigned char medianGreen = *medianIter;

		medianIter = blue.begin() + blue.size() / 2;
		std::nth_element(blue.begin(), medianIter, blue.end());
		unsigned char medianBlue = *medianIter;

		for (int row = 0; row != image.rows; ++row) {
			for (int col = 0; col != image.cols; ++col) {
				if (mask.at<unsigned char>(row, col)) {
					ret.at<cv::Vec3b>(row, col) = cv::Vec3b(medianBlue, medianGreen, medianRed);
				}
			}
		}
	}

	return ret;
}
