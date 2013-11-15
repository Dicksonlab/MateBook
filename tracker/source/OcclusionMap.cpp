#include "OcclusionMap.hpp"

#include <iterator>

#include "score2prob.hpp"
#include "prob2logodd.hpp"
#include "../../common/source/mathematics.hpp"

OcclusionMap::OcclusionMap()
{
}

size_t OcclusionMap::size() const
{
	return begin.size();
}

void OcclusionMap::append(size_t begin, size_t end)
{
	this->begin.push_back(begin);
	this->end.push_back(end);
}

void OcclusionMap::prepend(size_t begin, size_t end)
{
	std::vector<size_t> beginCopy = this->begin;
	std::vector<size_t> endCopy = this->end;

	this->begin.clear();
	this->end.clear();

	this->begin.push_back(begin);
	this->end.push_back(end);

	std::copy(beginCopy.begin(), beginCopy.end(), std::back_inserter(this->begin));
	std::copy(endCopy.begin(), endCopy.end(), std::back_inserter(this->end));
}

void OcclusionMap::appendScores(float tPosScore, float tMovScore, float tBocScore, float tPosLogisticRegressionCoefficient, float tBocLogisticRegressionCoefficient)
{
	this->tPosScore.push_back(tPosScore);
	tPosProb.push_back(logistic(this->tPosScore.back() * tPosLogisticRegressionCoefficient));
	tPos.push_back(prob2logodd(tPosProb.back()));

	this->tMovScore.push_back(tMovScore);
	tMovProb.push_back(score2prob(this->tMovScore.back()));
	tMov.push_back(prob2logodd(tMovProb.back()));
	
	// tBocScore Attribute has already been computed outside the OcclusionMap, so we skip it here
	tBocProb.push_back(logistic(tBocScore * tBocLogisticRegressionCoefficient));
	tBoc.push_back(prob2logodd(tBocProb.back()));

	if (std::abs(tPos.back()) > std::abs(tBoc.back())) {
		tCombined.push_back(tPos.back());
	} else {
		tCombined.push_back(tBoc.back());
	}
}
