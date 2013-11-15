#ifndef drawFly_hpp
#define drawFly_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>

class Fly;

void drawFly(const Fly& fly, const std::vector<std::vector<cv::Point> >& bodyContours, const std::vector<std::vector<cv::Point> >& wingContours, cv::Mat image);

#endif
