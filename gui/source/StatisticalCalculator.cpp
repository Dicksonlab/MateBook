#include "StatisticalCalculator.hpp"
#include <algorithm>

StatisticalCalculator::StatisticalCalculator()
{
	mean = 0;
	standardDeviation = 0;
	standardError = 0;
	binData.push_back(std::make_pair(std::make_pair(0, 0), 0.0f));
}

StatisticalCalculator::~StatisticalCalculator()
{
}

void StatisticalCalculator::calculateStatisticalValues(std::vector<float> container, size_t bin, int minimumValue)
{
	mean = calculateMean(container);
	standardDeviation = calculateStandardDeviation(container, mean);
	standardError = calculateStandardError(container, standardDeviation);

	std::sort(container.begin(), container.end());
	binData = getBinVector(container, bin, minimumValue);
}

float StatisticalCalculator::calculateMean(const std::vector<float>& container)
{
	if(container.size() < 1){
		return -1;
	}
	return createSum(container)/container.size();
}

float StatisticalCalculator::calculateStandardDeviation(const std::vector<float>& container, float mean)
{
	if(container.size() == 0){
		return -1;
	}
	if(container.size()-1 == 0){
		return 0;
	}
	float standardDeviation = sqrt(getVarianceSuptotal(container, mean) / (container.size()-1));
	return standardDeviation;
}

float StatisticalCalculator::calculateStandardError(const std::vector<float>& container, float standardDeviation)
{
	if(container.size() == 0){
		return -1;
	}
	float sqrtContainerSize = sqrt((float)container.size());
	if(sqrtContainerSize < 1){
		return 0;
	}
	float standardError = standardDeviation / sqrtContainerSize;
	return standardError;
}

float StatisticalCalculator::calculateQuantile(std::vector<float> container, float p)
{	//TODO: nth_element
	if(container.size() < 1){
		return -1;
	}
	std::sort(container.begin(), container.end());
	if(container.size() > 3){
		float integer = (int) container.size()*p;
		float nxp = (float)container.size()*p;
		if(nxp == integer){
			return (container[container.size()*p-1] + container[container.size()*p])/2;
		}else{
			return container[container.size()*p];
		}
	}
	return 0;
}

// counts the number of values that are smaller than or equal a certain value (bin)
SongResults::binStruct StatisticalCalculator::getBinVector(std::vector<float> container, size_t bin, int minimumValue)
{
	SongResults::binStruct binData;

	size_t count = 0;
	size_t currBin = minimumValue + bin;
	std::vector<float> binVector;
	for(size_t i = 0; i != container.size(); ++i){
		if(container[i] <= currBin){
			++count;
		}else{
			binData.push_back(std::make_pair(std::make_pair(currBin-bin, currBin), 100.0f / container.size() * count));
			currBin = currBin + bin;
			count = 0;
			--i;
		}
	}
	if(container.size() > 0){
		binData.push_back(std::make_pair(std::make_pair(currBin-bin, currBin), 100.0f / container.size() * count)); // last one
	}

	return binData;
}

float StatisticalCalculator::getMean()
{
	return mean;
}

float StatisticalCalculator::getStandardDeviation()
{
	return standardDeviation;
}

float StatisticalCalculator::getStandardError()
{
	return standardError;
}

SongResults::binStruct StatisticalCalculator::getBinData()
{
	return binData;
}

float StatisticalCalculator::createSum(const std::vector<float>& container)
{
	float sum = 0;
	for(int i = 0; i < container.size(); ++i){
		sum += container[i];
	}
	return sum;
}

float StatisticalCalculator::getVarianceSuptotal(const std::vector <float>& container, float mean)	//TODO: doesn't need to be a separate function
{
	float subtotal = 0;
	for(std::vector <float>::const_iterator it = container.begin(); it != container.end(); ++it){
		float diffToMean = (*it) - mean;
		subtotal += diffToMean * diffToMean;
	}
	return subtotal;
}