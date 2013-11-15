#ifndef reconstruct_hpp
#define reconstruct_hpp

#include <vector>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "../../common/source/MyBool.hpp"

cv::Mat reconstruct(const cv::Mat& marker, const cv::Mat& mask, unsigned int conn);
std::vector<bool> reconstruct(const std::vector<bool>& marker, const std::vector<bool>& mask);	// 1D
std::vector<MyBool> reconstruct(const std::vector<MyBool>& marker, const std::vector<MyBool>& mask);	// 1D

std::vector<cv::Mat> parallelReconstruct(const std::vector<cv::Mat>& markers, const cv::Mat& mask);

#endif
