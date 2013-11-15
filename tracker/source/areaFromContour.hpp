#ifndef areaFromContour_hpp
#define areaFromContour_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <cmath>

template<class T>
double areaFromContour(const std::vector<cv::Point_<T> >& contour)
{
	double area = 0.0;

	for (size_t pointNumber = 0; pointNumber != contour.size(); ++pointNumber) {
		cv::Point_<T> currentPoint = contour[pointNumber];
		cv::Point nextPoint = (pointNumber + 1 != contour.size() ? contour[pointNumber + 1] : contour[0]);
		// parallelogram area based on the cross product of vectors (0,0) to currentPoint and (0,0) to nextPoint
		area += currentPoint.x * nextPoint.y - nextPoint.x * currentPoint.y;
	}

	// take the absolute value in case we traced the wrong way around the area
	// divide by 2 to go from parallelogram area to triangle area
	return std::abs(area) / 2;
}

#endif
