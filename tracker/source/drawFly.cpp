#include "drawFly.hpp"
#include "Fly.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>

void drawFly(const Fly& fly, const std::vector<std::vector<cv::Point> >& bodyContours, const std::vector<std::vector<cv::Point> >& wingContours, cv::Mat image)
{
	cv::Scalar color(
		std::max(fly.get_headingFromColor() / 20.0f, 0.0f) * 255,
		0,
		std::min(fly.get_headingFromColor() / 20.0f, 0.0f) * (-255)
	);
	color[0] = std::min(color[0] * 100, 255.0);
	color[1] = std::min(color[1] * 100, 255.0);
	color[2] = std::min(color[2] * 100, 255.0);

	// draw contours
	drawContours(image, bodyContours, -1, color, 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());
	drawContours(image, wingContours, -1, cv::Scalar(0, 255, 0), 1, 8, std::vector<cv::Vec4i>(), 2, cv::Point());

	// sanity checks
	//TODO: do we need them?
/*	if (ellipseBB.center.x < 0 || ellipseBB.center.x > sourceWidth - 1 ||
		ellipseBB.size.height < 0 ||
		ellipseBB.size.height > min(sourceWidth, sourceHeight) ||
		ellipseBB.size.width < 0 ||
		ellipseBB.size.width > min(sourceWidth, sourceHeight)) {
		std::cout << "ellipse did not pass sanity check: center: (" << ellipseBB.center.x << "," << ellipseBB.center.y << "), size: (" << ellipseBB.size.width << "," << ellipseBB.size.height << ")" << std::endl;
		continue;
	}
*/
	// draw ellipses
/*	ellipse(
		image,
		fly.getBodyCentroid(),
		Size2f(fly.getBodyMajorAxisLength() / 2, fly.getBodyMinorAxisLength() / 2),
		90 - fly.getBodyOrientation(),
		0, 360, color
	);
	ellipse(
		image,
		fly.getWingCentroid(),
		Size2f(fly.getWingMajorAxisLength() / 2, fly.getWingMinorAxisLength() / 2),
		90 - fly.getWingOrientation(),
		0, 360, color
	);
*/}
