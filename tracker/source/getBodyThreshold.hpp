#ifndef getBodyThreshold_hpp
#define getBodyThreshold_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <boost/shared_ptr.hpp>
#include <fstream>

unsigned char getBodyThreshold(cv::Mat foreground, boost::shared_ptr<std::ofstream> smoothHistogramFile/* = boost::shared_ptr<std::ofstream>()*/);

#endif
