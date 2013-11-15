#include "hungarian.hpp"
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>

std::vector<size_t> hungarian(const cv::Mat& costs)
{
	//TODO: implement the actual hungarian algorithm...for now we're cheating using an exhaustive search, which may be faster for small N
	assert(costs.rows == costs.cols && costs.type() == CV_32F);
	std::vector<size_t> indirection(costs.rows);
	for (int i = 0; i != indirection.size(); ++i) {
		indirection[i] = i;
	}

	std::vector<size_t> bestIndirection = indirection;
	float lowestCost = std::numeric_limits<float>::infinity();
	do {
		float thisCost = 0;
		for (int i = 0; i != indirection.size(); ++i) {
			thisCost += costs.at<float>(i, indirection[i]);
		}
		if (thisCost < lowestCost) {
			bestIndirection = indirection;
			lowestCost = thisCost;
		}
	} while (std::next_permutation(indirection.begin(), indirection.end()));

	return bestIndirection;
}
