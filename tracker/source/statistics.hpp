#ifndef statistics_hpp
#define statistics_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <cmath>

template<class T>
double mean(const cv::Mat& data)
{
	double sum = 0;
	size_t count = 0;
	for (cv::MatConstIterator_<T> iter = data.begin<T>(); iter != data.end<T>(); ++iter) {
		sum += (*iter);
		++count;
	}
	return sum / count;
}

template<class T>
double stddev(const cv::Mat& data)
{
	double mean = ::mean<T>(data);
	double sum = 0;
	size_t count = 0;
	for (cv::MatConstIterator_<T> iter = data.begin<T>(); iter != data.end<T>(); ++iter) {
		sum += (*iter - mean) * (*iter - mean);
		++count;
	}
	return std::sqrt(sum / count);
}

#endif
