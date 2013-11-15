#ifndef signTest_hpp
#define signTest_hpp

/*
Returns p-Value for hypothesis "left sample is larger".
*/

#include <algorithm>
#include <boost/math/distributions/normal.hpp>

template<class Iterator>
float signTest(Iterator leftBegin, Iterator leftEnd, Iterator rightBegin)
{
	size_t leftLarger = 0;
	size_t rightLarger = 0;
	
	while (leftBegin != leftEnd) {
		if (*leftBegin > *rightBegin) {
			++leftLarger;
		}
		if (*leftBegin < *rightBegin) {
			++rightLarger;
		}
		++leftBegin;
		++rightBegin;
	}
	
	size_t total = leftLarger + rightLarger;
	if (total == 0) {
		return 0.5;
	}

	//TODO: justification? see mySigntest.m
	boost::math::normal standardNormalDistribution;
	if (leftLarger > total / 2) {
		double u1 = 2.0 * (total + 0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		double u2 = 2.0 * (leftLarger - 0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		return boost::math::cdf(standardNormalDistribution, u1) - boost::math::cdf(standardNormalDistribution, u2);
	} else {
		double u1 = 2.0 * (leftLarger + 0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		double u2 = 2.0 * (-0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		return 1.0 - (boost::math::cdf(standardNormalDistribution, u1) - boost::math::cdf(standardNormalDistribution, u2));
	}
}

template<class Iterator, class InvalidIterator>
float signTest(Iterator leftBegin, Iterator leftEnd, Iterator rightBegin, InvalidIterator invalidBegin)
{
	size_t leftLarger = 0;
	size_t rightLarger = 0;
	size_t total = 0;
	
	while (leftBegin != leftEnd) {
		if (!(*invalidBegin)) {
			if (*leftBegin > *rightBegin) {
				++leftLarger;
			}
			if (*leftBegin < *rightBegin) {
				++rightLarger;
			}
			++total;
		}
		++leftBegin;
		++rightBegin;
		++invalidBegin;
	}
	
	if (total == 0) {
		return 0.5;
	}

	//TODO: justification? see mySigntest.m
	boost::math::normal standardNormalDistribution;
	if (leftLarger > total / 2) {
		double u1 = 2.0 * (total + 0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		double u2 = 2.0 * (leftLarger - 0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		return boost::math::cdf(standardNormalDistribution, u1) - boost::math::cdf(standardNormalDistribution, u2);
	} else {
		double u1 = 2.0 * (leftLarger + 0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		double u2 = 2.0 * (-0.5 - total / 2.0) / sqrt(static_cast<double>(total));
		return 1.0 - (boost::math::cdf(standardNormalDistribution, u1) - boost::math::cdf(standardNormalDistribution, u2));
	}
}

#endif
