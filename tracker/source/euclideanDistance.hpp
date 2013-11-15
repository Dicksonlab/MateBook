#ifndef euclideanDistance_hpp
#define euclideanDistance_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <cmath>

//TODO: remove this file entirely
template<class T>
T euclideanDistance(const cv::Point_<T>& first, const cv::Point_<T>& second)
{
	T dX = first.x - second.x;
	T dY = first.y - second.y;
	return std::sqrt(dX * dX + dY * dY);
}

#endif
