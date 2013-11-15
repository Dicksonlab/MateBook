#ifndef inpaint_hpp
#define inpaint_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

cv::Mat inpaint(const cv::Mat& image, const cv::Mat& mask);

#endif
