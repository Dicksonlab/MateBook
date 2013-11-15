#ifndef StatisticalCalculator_hpp
#define StatisticalCalculator_hpp

#include <vector>
#include <cmath>
#include "SongResults.hpp"

/*
Can calculate the mean, standard deviation, standard error and quantiles of a vector.
It is possible to calculate all statistical values (calculateStatisticalValues) at 
once and get the results by calling the according getter functions.
*/

class StatisticalCalculator
{

public:
	StatisticalCalculator();
	~StatisticalCalculator();

	void calculateStatisticalValues(std::vector<float> container, size_t bin, int minimumValue);

	float calculateMean(const std::vector<float>& container);
	float calculateStandardDeviation(const std::vector<float>& container, float mean);
	float calculateStandardError(const std::vector<float>& container, float standardDeviation);
	float calculateQuantile(std::vector<float> container, float p);

	SongResults::binStruct getBinVector(std::vector<float> container, size_t bin, int minimumValue);

	float getMean();
	float getStandardDeviation();
	float getStandardError();
	SongResults::binStruct getBinData();

private:
	float mean;
	float standardDeviation;
	float standardError;
	SongResults::binStruct binData;

	float createSum(const std::vector<float>& container);
	float getVarianceSuptotal(const std::vector<float>& container, float mean);

};

#endif
