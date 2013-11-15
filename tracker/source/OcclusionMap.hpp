#ifndef OcclusionMap_hpp
#define OcclusionMap_hpp

#include <vector>

class OcclusionMap {
public:
	OcclusionMap();
	size_t size() const;
	void append(size_t begin, size_t end);
	void prepend(size_t begin, size_t end);

	void appendScores(float tPosScore, float tMovScore, float tBocScore, float tPosLogisticRegressionCoefficient, float tBocLogisticRegressionCoefficient);

	std::vector<size_t> begin;	// STL-like index of the first frame of the occlusion
	std::vector<size_t> end;	// STL-like index of the first frame after (!) the occlusion

	// t-scores, t-probabilities and t-logodds
	std::vector<float> tPosScore;
	std::vector<float> tMovScore;

	std::vector<float> tPosProb;
	std::vector<float> tMovProb;
	std::vector<float> tBocProb;

	std::vector<float> tPos;	// positive when the first fly before is close to the first fly after the occlusion
	std::vector<float> tMov;	// positive when the first fly before moves in the same direction as the first fly after the occlusion
	std::vector<float> tBoc;	// positive when the first fly before shares its contour with the first fly after the occlusion

	std::vector<float> tCombined;

private:
};

#endif
