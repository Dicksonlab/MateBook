#ifndef hungarian_hpp
#define hungarian_hpp

#include "opencv2/core/core.hpp"

std::vector<size_t> hungarian(const cv::Mat& costs);	// costs has to be a square array of floats

#endif
