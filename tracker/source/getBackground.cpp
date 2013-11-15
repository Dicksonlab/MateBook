#include "getBackground.hpp"
#include "../../common/source/StrideIterator.hpp"
#include "../../common/source/ordfilt.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>
#include "../../mediawrapper/source/VideoFrame.hpp"

cv::Mat getBackground(mw::InputVideo& sourceVideo, const std::string& type)
{
	// configuration
	const unsigned int bgExtractionSpacing = 50;		// seconds between frames used for background extraction
	const unsigned int bgExtractionMaxMem = 512;	// MB to use for background extraction (at most)

	// get the meta data for the video
	int sourceWidth = sourceVideo.getFrameWidth();
	int sourceHeight = sourceVideo.getFrameHeight();
	double sourceFrameRate = sourceVideo.getFrameRate();
	unsigned int sourceFrameCount = sourceVideo.getNumberOfFrames();
	if (!(sourceWidth > 0 && sourceHeight > 0 && sourceFrameRate > 0 && sourceFrameCount > 0)) {
		throw std::runtime_error("error: video meta data did not pass sanity check");
	}

	// for background extraction choose evenly spaced frames
	int bytesPerFrame = sourceWidth * sourceHeight * 3;
	int bgExtractionFrameCount = sourceFrameCount / (sourceFrameRate * bgExtractionSpacing);
	// for very short videos use at least one frame
	if (bgExtractionFrameCount < 1) {
		bgExtractionFrameCount = 1;
	}
	// reduce the number of frames used if bgExtractionMaxMem is exceeded
	if (bytesPerFrame * bgExtractionFrameCount > bgExtractionMaxMem * 1024 * 1024) {
		bgExtractionFrameCount = bgExtractionMaxMem * 1024 * 1024 / bytesPerFrame;
	}
	if (bgExtractionFrameCount < 1) {
		throw std::runtime_error("error: memory limit (bgExtractionMaxMem) set too low for background extraction");
	}

	// load the frames into memory
	std::cout << "info: using " << static_cast<float>(bgExtractionFrameCount * sourceWidth * sourceHeight * 3) / 1024 / 1024 << " MB for background extraction" << std::endl;
	unsigned char* bgExtractionFrames = new unsigned char[bgExtractionFrameCount * sourceWidth * sourceHeight * 3];
	if (bgExtractionFrameCount == 1) {
		// special case: use the middle frame
		sourceVideo.seekApprox(sourceFrameCount / 2);
		if (unsigned char* frameData = sourceVideo.getFrameBuffer(PIX_FMT_BGR24)) {
			std::copy(frameData, frameData + bytesPerFrame, bgExtractionFrames);
		}
	} else {
		int offset = (sourceFrameRate * bgExtractionSpacing) / 2;
		for (unsigned int frame = 0; frame != bgExtractionFrameCount; ++frame) {
			sourceVideo.seekApprox(offset + frame * sourceFrameRate * bgExtractionSpacing);
			if (unsigned char* frameData = sourceVideo.getFrameBuffer(PIX_FMT_BGR24)) {
				std::copy(frameData, frameData + bytesPerFrame, bgExtractionFrames + frame * bytesPerFrame);
			}
		}
	}

	// calculate the median for each pixel
	cv::Mat bgMedian(cv::Size(sourceWidth, sourceHeight), CV_8UC3);
	unsigned int medianPos = bgExtractionFrameCount * 50.0 / 100.0;
	for (unsigned char* colorIter = bgExtractionFrames; colorIter != bgExtractionFrames + bytesPerFrame; ++colorIter) {
		StrideIterator<unsigned char*> iter(colorIter, bytesPerFrame);
		std::nth_element(iter, iter + medianPos, iter + bgExtractionFrameCount);
	}
	std::copy(bgExtractionFrames + medianPos * bytesPerFrame, bgExtractionFrames + (medianPos + 1) * bytesPerFrame, bgMedian.datastart);
	delete bgExtractionFrames;
	bgExtractionFrames = NULL;
	//cvtColor(bgMedian, bgMedian, CV_RGB2BGR);	// OpenCV functions like imshow expect BGR

	if (type == "moonwalk") {
		// median filter every row of every color channel using a 1-by-151 structuring element
		int filterWidth = 151;	// should be odd
		std::vector<ptrdiff_t> neighborhood;
		for (ptrdiff_t offset = -(filterWidth / 2); offset <= (filterWidth / 2); ++offset) {
			neighborhood.push_back(offset);
		}

		int rows = bgMedian.rows;
		int cols = bgMedian.cols;
		std::vector<unsigned char> sourceRow;
		sourceRow.reserve(bgMedian.cols);
		std::vector<cv::Mat> channels;
		cv::split(bgMedian, channels);
		for (std::vector<cv::Mat>::iterator channelIter = channels.begin(); channelIter != channels.end(); ++channelIter) {
			cv::Mat& thisChannel = *channelIter;
			for (int rowIndex = 0; rowIndex != thisChannel.rows; ++rowIndex) {
				unsigned char* channelRow = thisChannel.ptr<unsigned char>(rowIndex);
				sourceRow.insert(sourceRow.end(), channelRow, channelRow + cols);
				std::vector<unsigned char> filteredRow = ordfilt_mirror(sourceRow, 0.5, neighborhood);
				std::copy(filteredRow.begin(), filteredRow.end(), channelRow);
				sourceRow.clear();
			}
		}
		cv::merge(channels, bgMedian);
	}

	return bgMedian;
}
