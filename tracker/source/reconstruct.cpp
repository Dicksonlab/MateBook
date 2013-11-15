#include "reconstruct.hpp"
#include <stdexcept>
#include <vector>
#include "../../common/source/arrayOperations.hpp"

cv::Mat reconstruct(const cv::Mat& marker, const cv::Mat& mask, unsigned int conn)
{
	if (marker.size() != mask.size()) {
		throw std::runtime_error("reconstruct: marker and mask sizes don't match");
	}

	if (conn != 4 && conn != 8) {
		throw std::runtime_error("reconstruct: connectivity needs to be 4 or 8");
	}

	cv::Mat result = marker.clone();

	if (!(marker.isContinuous() && mask.isContinuous() && result.isContinuous())) {
		throw std::runtime_error("reconstruct: requires continuous Mat");
	}

	//TODO: this is not elegant - we're switching width and height because images are stored row-wise in OpenCV
	const size_t colCount = marker.size().height;
	const size_t rowCount = marker.size().width;
	const size_t elementCount = colCount * rowCount;
	
	std::vector<size_t> offsetsToFill; // we'll use this like a stack
	offsetsToFill.reserve(elementCount / 16); // seems reasonable
	
	const bool* markerBegin = marker.ptr<bool>();
	const bool* maskBegin = mask.ptr<bool>();
	bool* resultBegin = result.ptr<bool>();

	// push the offsets of the pixels which are set in the marker onto the stack
	for (size_t offset = 0; offset != elementCount; ++offset) {
		if (*(markerBegin + offset)) {
			offsetsToFill.push_back(offset);
		}
	}

	if (conn == 4) {
		while (!offsetsToFill.empty()) {
			// fill this pixel
			size_t offset = offsetsToFill.back();
			offsetsToFill.pop_back();
			*(resultBegin + offset) = true;
			
			if (offset >= rowCount) { // we may go left
				size_t nextOffset = offset - rowCount;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
			}
			if (offset + rowCount < elementCount) { // we may go right
				size_t nextOffset = offset + rowCount;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
			}
			if (offset % rowCount != 0) { // going up...
				size_t nextOffset = offset - 1;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
			}
			if ((offset + 1) % rowCount != 0) { // ...going down
				size_t nextOffset = offset + 1;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
			}
		}
	} else if (conn == 8) {
		while (!offsetsToFill.empty()) {
			// fill this pixel
			size_t offset = offsetsToFill.back();
			offsetsToFill.pop_back();
			*(resultBegin + offset) = true;
			
			bool goLeft = (offset >= rowCount);
			bool goRight = (offset + rowCount < elementCount);
			bool goUp = (offset % rowCount != 0);
			bool goDown = ((offset + 1) % rowCount != 0);
			
			if (goLeft) {
				size_t nextOffset = offset - rowCount;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
				
				if (goUp) {
					if (!*(resultBegin + nextOffset - 1) && *(maskBegin + nextOffset - 1)) {
						offsetsToFill.push_back(nextOffset - 1);
					}
				}

				if (goDown) {
					if (!*(resultBegin + nextOffset + 1) && *(maskBegin + nextOffset + 1)) {
						offsetsToFill.push_back(nextOffset + 1);
					}
				}
			}
			if (goRight) {
				size_t nextOffset = offset + rowCount;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
				
				if (goUp) {
					if (!*(resultBegin + nextOffset - 1) && *(maskBegin + nextOffset - 1)) {
						offsetsToFill.push_back(nextOffset - 1);
					}
				}

				if (goDown) {
					if (!*(resultBegin + nextOffset + 1) && *(maskBegin + nextOffset + 1)) {
						offsetsToFill.push_back(nextOffset + 1);
					}
				}
			}
			if (goUp) {
				size_t nextOffset = offset - 1;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
			}
			if (goDown) {
				size_t nextOffset = offset + 1;
				if (!*(resultBegin + nextOffset) && *(maskBegin + nextOffset)) {
					offsetsToFill.push_back(nextOffset);
				}
			}
		}
	}

	return result;
}

std::vector<bool> reconstruct(const std::vector<bool>& marker, const std::vector<bool>& mask)
{
	if (marker.size() != mask.size()) {
		throw std::runtime_error("reconstruct: marker and mask sizes don't match");
	}

	std::vector<bool> result = marker;
	const size_t elementCount = result.size();

	std::vector<size_t> offsetsToFill; // we'll use this like a stack
	offsetsToFill.reserve(elementCount / 16); // seems reasonable
	
	// push the offsets of the pixels which are set in the marker onto the stack
	for (size_t offset = 0; offset != elementCount; ++offset) {
		if (marker[offset]) {
			offsetsToFill.push_back(offset);
		}
	}

	while (!offsetsToFill.empty()) {
		// fill this pixel
		size_t offset = offsetsToFill.back();
		offsetsToFill.pop_back();
		result[offset] = true;
		
		if (offset > 0) { // we may go left
			size_t nextOffset = offset - 1;
			if (!result[nextOffset] && mask[nextOffset]) {
				offsetsToFill.push_back(nextOffset);
			}
		}
		if (offset + 1 < elementCount) { // we may go right
			size_t nextOffset = offset + 1;
			if (!result[nextOffset] && mask[nextOffset]) {
				offsetsToFill.push_back(nextOffset);
			}
		}
	}

	return result;
}

std::vector<MyBool> reconstruct(const std::vector<MyBool>& marker, const std::vector<MyBool>& mask)
{
	if (marker.size() != mask.size()) {
		throw std::runtime_error("reconstruct: marker and mask sizes don't match");
	}

	std::vector<MyBool> result = marker;
	const size_t elementCount = result.size();

	std::vector<size_t> offsetsToFill; // we'll use this like a stack
	offsetsToFill.reserve(elementCount / 16); // seems reasonable
	
	// push the offsets of the pixels which are set in the marker onto the stack
	for (size_t offset = 0; offset != elementCount; ++offset) {
		if (marker[offset]) {
			offsetsToFill.push_back(offset);
		}
	}

	while (!offsetsToFill.empty()) {
		// fill this pixel
		size_t offset = offsetsToFill.back();
		offsetsToFill.pop_back();
		result[offset] = true;
		
		if (offset > 0) { // we may go left
			size_t nextOffset = offset - 1;
			if (!result[nextOffset] && mask[nextOffset]) {
				offsetsToFill.push_back(nextOffset);
			}
		}
		if (offset + 1 < elementCount) { // we may go right
			size_t nextOffset = offset + 1;
			if (!result[nextOffset] && mask[nextOffset]) {
				offsetsToFill.push_back(nextOffset);
			}
		}
	}

	return result;
}

std::vector<cv::Mat> parallelReconstruct(const std::vector<cv::Mat>& markers, const cv::Mat& mask)
{
	std::vector<cv::Mat> split;
	split.reserve(markers.size());
	for (size_t i = 0; i != markers.size(); ++i) {
		split.push_back(markers[i].clone());
	}

	std::vector<cv::Mat> last;
	last.reserve(markers.size());
	for (size_t i = 0; i != markers.size(); ++i) {
		last.push_back(markers[i].clone());
	}

	std::vector<bool> cannotChange(markers.size());	// the area hasn't changed in a previous iteration so it will not change in a future iteration either and we don't even have to try
	while (true) {
		for (size_t i = 0; i != split.size(); ++i) {
			if (cannotChange[i]) {
				continue;
			}
			cv::dilate(last[i], split[i], cv::Mat());
			split[i] = split[i] & mask;

			for (size_t j = 0; j < i; ++j) {
				split[i] = split[i] & ~split[j];
			}
			for (size_t j = i + 1; j < split.size(); ++j) {
				split[i] = split[i] & ~last[j];
			}

			// test if split is different from last
			//TODO: is there really no operator== for Mat? get rid of OpenCV...
			bool hasChanged = false;
			for (int row = 0; row != split[i].rows; ++row) {
				for (int col = 0; col != split[i].cols; ++col) {
					if ((split[i].at<unsigned char>(row, col) == 0 && last[i].at<unsigned char>(row, col) != 0) || (split[i].at<unsigned char>(row, col) != 0 && last[i].at<unsigned char>(row, col) == 0)) {	// just to be sure
						hasChanged = true;
						break;
					}
				}
				if (hasChanged) {
					break;
				}
			}
			if (!hasChanged) {
				cannotChange[i] = true;
			}
		}

		// break if there are no areas left that can change
		if (all(cannotChange.begin(), cannotChange.end())) {
			break;
		}

		std::swap(split, last);
	}

	return split;
}
