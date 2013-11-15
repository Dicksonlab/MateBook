#ifndef SongVisitor_hpp
#define SongVisitor_hpp

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <set>

#include "SongResults.hpp"

class Item;

/**
  * @class  SongVisitor
  * @brief  gathers results (not a true visitor: no hierarchy and many dynamic_cast<> operations)
  */
class SongVisitor {
public:
	int visit(const Item* item);
	void writeReport(const std::string& fileName);

private:
	struct SongFile{
		std::string fileName;
		int			duration;
		int			pulsesPerMinute;
		int			pulsesPerMinuteExclude;
		int			pulseCount;
		float		meanCPP;
		float		standardDeviationCPP;
		float		standardErrorCPP;
		int			ipiCount;
		float		meanIPI;
		float		standardDeviationIPI;
		float		standardErrorIPI;
		int			trainCount;
		float		meanTrain;
		float		standardDeviationTrain;
		float		standardErrorTrain;
		int			sineEpisodeCount;
		float		meanSineDuration;
		float		totalSineDuration;

		std::string	experimenter;
		std::string	sex1;
		std::string	genotype1;
		std::string	sex2;
		std::string	genotype2;
		std::string comment;
		
		std::map<std::string, SongResults::binStruct> bins;
		std::map<QString, float> pulsedetectOptions;
		std::map<QString, float> statisticalOptions;
	};

	std::vector<SongFile> songFiles;
	std::set<const Item*> visitedItems;

	void writeStatisticalBinningValues(std::ofstream& stream, const std::vector<float>& data, std::string name);
	void calculateBinStatistics(const std::vector<SongResults::binStruct>& source, std::vector <float>& mean, std::vector <float>& error);
};

#endif
