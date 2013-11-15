#ifndef findArenas_hpp
#define findArenas_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>

#include "Arena.hpp"
#include "Shape.hpp"
#include "Interior.hpp"

std::vector<Arena> findArenas(const cv::Mat& image, Shape shape, double sourceFrameRate, size_t flyCount, float diameter, float borderSize, Interior interior);
cv::Mat drawArenaMask(cv::Size size, Shape shape, float borderSize);

#endif
