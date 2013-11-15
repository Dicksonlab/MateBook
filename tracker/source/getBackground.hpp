#ifndef getBackground_hpp
#define getBackground_hpp

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "../../mediawrapper/source/mediawrapper.hpp"

cv::Mat getBackground(mw::InputVideo& video, const std::string& type);

#endif
