#ifndef findCircles_hpp
#define findCircles_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>

#include "Interior.hpp"

std::vector<cv::Vec3f> findCircles(const cv::Mat& image, Interior interior);

#endif
