#include "SongResults.hpp"
#include <QStringList>
#include <QFile>

#include <cmath>
#include <numeric>
#include "../../common/source/Settings.hpp"
#include "StatisticalCalculator.hpp"
#include "Video.hpp"

SongResults::SongResults() :
	fileName(),
	pulseCenters(),
	pulseCycles(),
	ipi(),
	pulses(),
	trains(),
	sines(),
	pulsePerMinuteCount(0)
{
}

SongResults::SongResults(const std::string& filepath) :
	fileName(filepath),
	pulseCenters(),
	pulseCycles(),
	ipi(),
	pulses(),
	trains(),
	sines(),
	pulsePerMinuteCount(0)
{
}

SongResults::~SongResults()
{
}

//TODO: return peak so it can be drawn
// returns whether the pulse was created
bool SongResults::createPulse(const Video& media, std::pair<int, int> indexRange)
{
	const std::vector<float>& currentSong = media.getSong();

	std::map<size_t, size_t>::const_iterator it_next(pulses.lower_bound(indexRange.first));
	std::map<size_t, size_t>::const_iterator it_prev = it_next;
	if(it_next != pulses.end()) --it_prev;

	int lower = pulses.size() > 0 && it_prev != pulses.end()? (*it_prev).second : -1;
	int upper = pulses.size() > 0 && it_next != pulses.end()? (*it_next).first : currentSong.size();
	// in function selectPulses(): already tested if they contain an index within song range
	// only creates pulse if no other pulse is selcted
	if(upper > indexRange.second && lower < indexRange.first){
		if(indexRange.first != indexRange.second){
			pulses.insert(pulses.begin(), std::pair<size_t, size_t>(indexRange.first, indexRange.second)); 

			size_t biggest = indexRange.first;
			for(size_t i = indexRange.first+1; i < currentSong.size() && i <= indexRange.second; ++i){
				if(abs(currentSong[i]) > abs(currentSong[biggest])){
					biggest = i;
				}
			}
			pulseCenters.insert(pulseCenters.begin(), std::pair<size_t, size_t>(indexRange.first, biggest)); 
			pulseCycles.insert(pulseCycles.begin(), std::pair<size_t, size_t>(indexRange.first, countPeakCycles(media, indexRange.first, indexRange.second)));
			return true;
		}
	}

	return false;
}

// returns whether pulses were deleted
bool SongResults::deletePulses(std::pair<int, int> indexRange)
{
	// get current selected pulses and pulse centers: iterates over all indizes of the  
	// given range (selectedIndexes) and checks if there is a starting index of a pulse 
	// (lower_bound) if a pulse was found it is saved in selectedPulses and the iterator 
	// jumps to the start of the found pulse (because overlapping of pulses is possible  
	// - otherwise it should jump to the end)
	// a signal is sent to announce which pulse is selected

	

	std::map<size_t, size_t> selectedPulses;

	for (int i = indexRange.first; i < indexRange.second;) {
		std::map<size_t, size_t>::const_iterator it = pulses.lower_bound(i);
		if (it == pulses.end()) {
			break;
		}
		if (it->second > indexRange.second) {	// pulse not fully contained in selection
			break;
		}
		selectedPulses[it->first] = it->second;
		i = it->second;
	}

	if (selectedPulses.size()) {
		for (std::map<size_t, size_t>::iterator it = selectedPulses.begin(); it != selectedPulses.end(); ++it) {
			pulses.erase(it->first);
			pulseCenters.erase(it->first);
			pulseCycles.erase(it->first);
		}
		return true;
	}

	return false;
}

// returns whether the pulse was created
bool SongResults::createSine(const Video& media, std::pair<int, int> indexRange)
{
	const std::vector<float>& currentSong = media.getSong();

	std::map<size_t, size_t>::const_iterator it_next(sines.lower_bound(indexRange.first));
	std::map<size_t, size_t>::const_iterator it_prev = it_next;
	if(it_next != sines.end()) --it_prev;

	int lower = sines.size() > 0 && it_prev != sines.end()? (*it_prev).second : -1;
	int upper = sines.size() > 0 && it_next != sines.end()? (*it_next).first : currentSong.size();
	// in function selectPulses(): already tested if they contain an index within song range
	// only creates pulse if no other pulse is selcted
	if(upper > indexRange.second && lower < indexRange.first){
		if(indexRange.first != indexRange.second){
			sines.insert(sines.begin(), std::pair<size_t, size_t>(indexRange.first, indexRange.second)); 
			return true;
		}
	}

	return false;
}

// returns whether pulses were deleted
bool SongResults::deleteSines(std::pair<int, int> indexRange)
{
	std::map<size_t, size_t> selectedSines;

	for (int i = indexRange.first; i < indexRange.second;) {
		std::map<size_t, size_t>::const_iterator it = sines.lower_bound(i);
		if (it == sines.end()) {
			break;
		}
		if (it->second > indexRange.second) {	// sine not fully contained in selection
			break;
		}
		selectedSines[it->first] = it->second;
		i = it->second;
	}

	if (selectedSines.size()) {
		for (std::map<size_t, size_t>::iterator it = selectedSines.begin(); it != selectedSines.end(); ++it) {
			sines.erase((*it).first);
		}
		return true;
	}

	return false;
}

const std::map<size_t, size_t>& SongResults::getPulses() const
{
	return pulses;
}

const std::map<size_t, size_t>& SongResults::getSines() const
{
	return sines;
}

const std::map<size_t, size_t>& SongResults::getTrains() const
{
	return trains;
}

const std::map<size_t, size_t>& SongResults::getIPI() const
{
	return ipi;
}

const std::map<size_t, size_t>& SongResults::getPulseCycles() const
{
	return pulseCycles;
}

const std::map<size_t, size_t>& SongResults::getPulseCenters() const
{
	return pulseCenters;
}

float SongResults::getStatisticalValue(std::string name)
{
	return statisticalValues[name];
}

SongResults::binStruct SongResults::getBinData(std::string name)
{
	return binData[name];
}

void SongResults::setFilePath(const std::string& fileName)
{
	this->fileName = fileName;
}

void SongResults::setStatisticalValue(std::string name, float value)
{
	statisticalValues[name] = value;
}

void SongResults::setBinData(std::string name, const binStruct& data)
{
	binData[name] = data;
}

void SongResults::writeStatisticFile()
{
	writePairFile<std::string, float>(statisticalValues, std::string(fileName + "/songstatistics.txt").c_str());

	for(std::map<std::string, binStruct>::iterator iter = binData.begin(); iter != binData.end(); ++iter){
		writeBinningFile((*iter).second, std::string(fileName + "/song" + (*iter).first +".txt").c_str());
	}
}

void SongResults::writeFileCleanedFile()
{
	std::map<std::string, std::string> data;
	data["was"] = "cleaned";
	writePairFile<std::string, std::string>(data, std::string(fileName + "/songfile_was_cleaned.txt").c_str());
}

void SongResults::writePulseDetectionOptions(const std::map<QString, float> &options)
{
	std::map<std::string, float> temp;
	for(std::map<QString, float>::const_iterator iter = options.begin(); iter != options.end(); ++iter){
		temp[(*iter).first.toStdString()] = (*iter).second;
	}
	writePairFile<std::string, float>(temp, std::string(fileName + "/pulsedetectionoptions.txt").c_str());
}

void SongResults::writeSongStatisticsOptions(const std::map<QString, float> &options)
{
	std::map<std::string, float> temp;
	for(std::map<QString, float>::const_iterator iter = options.begin(); iter != options.end(); ++iter){
		temp[(*iter).first.toStdString()] = (*iter).second;
	}
	writePairFile<std::string, float>(temp, std::string(fileName + "/songstatisticsoptions.txt").c_str());
}

void SongResults::readStatisticFile()
{
	statisticalValues = readPairFile<std::string, float>(std::string(fileName + "/songstatistics.txt").c_str());
	binData["binIPI"] = readBinningFile(std::string(fileName + "/songbinIPI.txt").c_str());
	binData["binCycles"] = readBinningFile(std::string(fileName + "/songbinCycles.txt").c_str());
	binData["binTrains"] = readBinningFile(std::string(fileName + "/songbinTrains.txt").c_str());
}

std::map<QString, float> SongResults::readPulseDetectionOptions()
{
	std::map<std::string, float> temp = readPairFile<std::string, float>(std::string(fileName + "/pulsedetectionoptions.txt").c_str());
	std::map<QString, float> options;
	for(std::map<std::string, float>::const_iterator iter = temp.begin(); iter != temp.end(); ++iter){
		options[QString::fromStdString((*iter).first)] = (*iter).second;
	}
	return options;
}

std::map<QString, float> SongResults::readSongStatisticsOptions()
{
	std::map<std::string, float> temp = readPairFile<std::string, float>(std::string(fileName + "/songstatisticsoptions.txt").c_str());
	std::map<QString, float> options;
	for(std::map<std::string, float>::const_iterator iter = temp.begin(); iter != temp.end(); ++iter){
		options[QString::fromStdString((*iter).first)] = (*iter).second;
	}
	return options;
}

void SongResults::reloadData()
{
	pulses = readPairFile<size_t, size_t>(std::string(fileName + "/songpulses.txt").c_str());
	sines = readPairFile<size_t, size_t>(std::string(fileName + "/songsines.txt").c_str());	
	trains = readPairFile<size_t, size_t>(std::string(fileName + "/songtrains.txt").c_str());
	ipi = readPairFile<size_t, size_t>(std::string(fileName + "/songipi.txt").c_str());
	pulseCenters = readPairFile<size_t, size_t>(std::string(fileName + "/songpulsecenters.txt").c_str());
	pulseCycles = readPairFile<size_t, size_t>(std::string(fileName + "/songpulsecycles.txt").c_str());
	readStatisticFile();
}

void SongResults::reloadRawData()
{
	deleteStatisticFilesAndData();
	copyFile(std::string(fileName + "/raw_songpulses.txt").c_str(), std::string(fileName + "/songpulses.txt").c_str());
	copyFile(std::string(fileName + "/raw_songsines.txt").c_str(), std::string(fileName + "/songsines.txt").c_str());
	copyFile(std::string(fileName + "/raw_songtrains.txt").c_str(), std::string(fileName + "/songtrains.txt").c_str());
	copyFile(std::string(fileName + "/raw_songpulsecenters.txt").c_str(), std::string(fileName + "/songpulsecenters.txt").c_str());
	copyFile(std::string(fileName + "/raw_songpulsecycles.txt").c_str(), std::string(fileName + "/songpulsecycles.txt").c_str());
	QFile::remove(std::string(fileName + "/songfile_was_cleaned.txt").c_str());
	reloadData();
}

// sines can not be modified yet so no need to load them
void SongResults::reloadCleanData()
{
	deleteStatisticFilesAndData();
	copyFile(std::string(fileName + "/clean_songpulses.txt").c_str(), std::string(fileName + "/songpulses.txt").c_str());
	copyFile(std::string(fileName + "/clean_songsines.txt").c_str(), std::string(fileName + "/songsines.txt").c_str());
	copyFile(std::string(fileName + "/clean_songtrains.txt").c_str(), std::string(fileName + "/songtrains.txt").c_str());
	copyFile(std::string(fileName + "/clean_songpulsecenters.txt").c_str(), std::string(fileName + "/songpulsecenters.txt").c_str());
	copyFile(std::string(fileName + "/clean_songpulsecycles.txt").c_str(), std::string(fileName + "/songpulsecycles.txt").c_str());
	writeFileCleanedFile();
	reloadData();
}

void SongResults::saveData()
{
	writePairFile<size_t, size_t>(pulses, std::string(fileName + "/songpulses.txt").c_str());
	writePairFile<size_t, size_t>(pulseCenters, std::string(fileName + "/songpulsecenters.txt").c_str());
	writePairFile<size_t, size_t>(pulseCycles, std::string(fileName + "/songpulsecycles.txt").c_str());
	writePairFile<size_t, size_t>(sines, std::string(fileName + "/songsines.txt").c_str());
	writePairFile<size_t, size_t>(trains, std::string(fileName + "/songtrains.txt").c_str());
	writePairFile<size_t, size_t>(ipi, std::string(fileName + "/songipi.txt").c_str());
}

void SongResults::saveCleanData()
{
	writePairFile<size_t, size_t>(pulses, std::string(fileName + "/clean_songpulses.txt").c_str());
	writePairFile<size_t, size_t>(pulseCenters, std::string(fileName + "/clean_songpulsecenters.txt").c_str());
	writePairFile<size_t, size_t>(pulseCycles, std::string(fileName + "/clean_songpulsecycles.txt").c_str());
	writePairFile<size_t, size_t>(sines, std::string(fileName + "/clean_songsines.txt").c_str());
	writePairFile<size_t, size_t>(trains, std::string(fileName + "/clean_songtrains.txt").c_str());
	//TODO: what about IPI?
}

void SongResults::clearAllData()
{
	pulseCenters.clear();
	pulseCycles.clear();
	ipi.clear();
	pulses.clear();
	sines.clear();
	trains.clear();
	statisticalValues.clear();
	binData.clear();
	pulsePerMinuteCount = 0;
	pulsePerMinuteAllCount = 0;
}

void SongResults::calculateStatisticalValues(std::map<QString, float> statisticOptions, size_t sampleRate, float samples, size_t startSample, size_t endSample)
{
	deleteStatisticFilesAndData();
	StatisticalCalculator calculator;

	float minDistanceIPI = statisticOptions["MinIPIdistance"];
	float binIPI = statisticOptions["IPIbin"];
	float binTrain = statisticOptions["Trainbin"];
	size_t minTTrainLength = statisticOptions["Train:MinTrainLength"];
	size_t ipiMinThreshold = (float) sampleRate / 1000 * minDistanceIPI;
	size_t ipiMaxThreshold = (float) sampleRate / 1000 * statisticOptions["MaxIPIdistance"];

	detectTrains(statisticOptions["Train:MinTrainLength"], statisticOptions["IPI:MinTrainLength"], statisticOptions["Cycle:MinTrainLength"], statisticOptions["PulsePerMinuteExclude"], ipiMaxThreshold, ipiMinThreshold, startSample, endSample);

	if(pulseCycles.size() > 0 && pulseCycles.size() == pulses.size()){
		std::vector<float> ipiTime;
		std::vector<float> trainsTime;
		std::vector<float> cycleTime;
		ipiTime.reserve(ipi.size());
		trainsTime.reserve(trains.size());
		cycleTime.reserve(pulseCycles.size());

		// get ipi times in milliseconds and the total sum
		for(std::map<size_t, size_t>::const_iterator it = ipi.begin(); it != ipi.end(); ++it){
				ipiTime.push_back(1000.0 / sampleRate * (*it).second);
		}

		// get the total sum of cycles
		for(std::map<size_t, size_t>::const_iterator it = pulseCycles.begin(); it != pulseCycles.end(); ++it){
			if((*it).second >= statisticOptions["MinCycleNumber"]){
				cycleTime.push_back((*it).second);
			}
		}

		// get the total sum of train pulses
		for(std::map<size_t, size_t>::const_iterator it = trains.begin(); it != trains.end(); ++it){
				std::map <size_t, size_t>::const_iterator startPulse = pulses.find((*it).first);
				std::map <size_t, size_t>::const_iterator endPulse = pulses.find((*it).second);
				trainsTime.push_back(std::distance(startPulse, endPulse)+1);
		}
		
		float songDuration = ((float)(endSample - startSample) / (float) sampleRate * 1000.0) / 60000.0;
		float pulsePerMin = pulses.size() > 0 && songDuration > 0? pulsePerMinuteAllCount/songDuration : -1;
		float pulsePerMinExclude = pulses.size() > 0 && songDuration > 0? pulsePerMinuteCount/songDuration : -1;
		setStatisticalValue("pulsesPerMinute", pulsePerMin);
		setStatisticalValue("pulsesPerMinuteExclude", pulsePerMinExclude);
		setStatisticalValue("PulsesForCycles", cycleTime.size());

		calculator.calculateStatisticalValues(ipiTime, binIPI, minDistanceIPI);
		setStatisticalValue("meanIPI", calculator.getMean());
		setStatisticalValue("standardDeviationIPI", calculator.getStandardDeviation());
		setStatisticalValue("standardErrorIPI", calculator.getStandardError());
		setBinData("binIPI", calculator.getBinData());
		setStatisticalValue("quartil25IPI", calculator.calculateQuantile(ipiTime, 0.25));
		setStatisticalValue("quartil50IPI", calculator.calculateQuantile(ipiTime, 0.5));
		setStatisticalValue("quartil75IPI", calculator.calculateQuantile(ipiTime, 0.75));

		calculator.calculateStatisticalValues(cycleTime, 1, -1);
		setStatisticalValue("meanCPP", calculator.getMean());
		setStatisticalValue("standardDeviationCycle", calculator.getStandardDeviation());
		setStatisticalValue("standardErrorCycle", calculator.getStandardError());
		setBinData("binCycles", calculator.getBinData());

		calculator.calculateStatisticalValues(trainsTime, binTrain, minTTrainLength);
		setStatisticalValue("meanTrain", calculator.getMean());
		setStatisticalValue("standardDeviationTrain", calculator.getStandardDeviation());
		setStatisticalValue("standardErrorTrain", calculator.getStandardError());
		setBinData("binTrains", calculator.getBinData());
	}

	if (sines.size() > 0) {
		std::vector<float> sineDurations;

		// get sine song episode durations in milliseconds
		float totalMs = 0;
		for (std::map<size_t, size_t>::const_iterator it = sines.begin(); it != sines.end(); ++it) {
			float thisDurationMs = 1000.0 / sampleRate * (it->second - it->first);
			totalMs += thisDurationMs;
			sineDurations.push_back(thisDurationMs);
		}

		StatisticalCalculator calculator;

		setStatisticalValue("meanSineDuration", calculator.calculateMean(sineDurations));
		setStatisticalValue("totalSineDuration", totalMs);
		setStatisticalValue("sineEpisodeCount", sineDurations.size());
	}

	QFile(std::string(fileName + "/songstatistics_done_success.txt").c_str()).open(QIODevice::WriteOnly);
	writeSongStatisticsOptions(statisticOptions);
}

// if two pulse centers have a distance within a user defined threshold and there are
// a minimum number of pulses in range (train), the pulses are counted as pulse
void SongResults::detectTrains(size_t minTTrainLength, size_t minITrainLength, size_t minCTrainLength, size_t pulsePerMinuteExclude, size_t maxIPIdistance, size_t minIPIdistance, size_t startSample, size_t endSample)
{
	std::map <size_t, size_t> newPulses;
	std::map <size_t, size_t> newTrains;
	std::map <size_t, size_t> newIPI;
	std::map <size_t, size_t> newCenters;
	std::map <size_t, size_t> newCycles;

	if(pulseCenters.size() > 0 && pulseCenters.size() == pulses.size() && pulseCenters.size() == pulseCycles.size()){
		std::vector <size_t> tempCenters;
		std::vector <size_t> tempDistance;
		
		std::map<size_t, size_t>::iterator pIt = pulses.begin();
		std::map<size_t, size_t>::iterator prev = pulseCenters.begin();
		std::map<size_t, size_t>::iterator curr = prev;

		if(curr != pulseCenters.end()){
			++curr; // curr is always one ahead of prev
		}
		bool finished = false;
		pulsePerMinuteCount = 0;
		pulsePerMinuteAllCount = 0;

		// if the distance between two pulse centers is lower than the maxIPIdistance and not zero
		// the distance is a possible ipi and temporarely saved. As soon as one distance is bigger
		// than the maxIPIdistance the last center is also added to the temporary saved ones.
		// it is individually tested if the number of saved centers is bigger than than the minimalTrainLength
		// for trains, for centers and for ipis. If that is the case, these centers are taken into account
		// for the statistical analysis
		// the rest is not counted as relevant data
		while(!finished){
			size_t frameDistance(0);
			if((*prev).first > startSample && pulses.find((*prev).first)->second < endSample && curr != pulseCenters.end() && 
				(frameDistance = (*curr).second - (*prev).second) < maxIPIdistance && frameDistance > minIPIdistance && frameDistance != 0){ //0 if they have same center -> overlapping pulses
					tempCenters.push_back((*prev).second);
					if((*curr).first > startSample && pulses.find((*curr).first)->second < endSample){
						tempDistance.push_back(frameDistance);
					}
			}else{
				if((*prev).first > startSample && pulses.find((*prev).first)->second < endSample){
					tempCenters.push_back((*prev).second); // last one: belongs to curr train
				}
				pulsePerMinuteAllCount += tempCenters.size();

				if(tempCenters.size() >= minTTrainLength){ // new trains
					std::map <size_t, size_t>::iterator tempIt = pIt;
					size_t trainStart = (*tempIt).first;
					std::advance(tempIt,tempCenters.size()-1);
					newTrains[trainStart] = (*tempIt).first;
				}
				if(pulsePerMinuteExclude < tempCenters.size()){
					pulsePerMinuteCount += tempCenters.size();
				}
				if(tempCenters.size() >= minITrainLength){ // new ipis
					std::map <size_t, size_t>::iterator tempIt = pIt;
					for(int j = 0; j < tempDistance.size(); ++j){
						newIPI[(*tempIt).first] = tempDistance[j];
						++tempIt;
					}
				}
				if(tempCenters.size() >= minCTrainLength){ // new centers
					for(int j = 0; j < tempCenters.size() && j < pulseCycles.size(); ++j){
						newCycles[(*pIt).first] = pulseCycles.find((*pIt).first)->second;
						newCenters[(*pIt).first] = pulseCenters.find((*pIt).first)->second;
						newPulses[(*pIt).first] = (*pIt).second;
						++pIt;
					}
				}else{
					for(int j = 0; j < tempCenters.size() && pIt != pulses.end(); ++j){
						++pIt;
					}
				}
				tempCenters.clear();
				tempDistance.clear();
			}
			if(curr == pulseCenters.end()){
				finished = true;
			}else{
				++curr;
				++prev;
			}
		}
	}
	setStatisticalValue("pulsesPerMinuteExclude", pulsePerMinuteCount);
	pulses = newPulses;
	pulseCenters = newCenters;
	pulseCycles = newCycles;
	ipi = newIPI;
	trains = newTrains;
}

void SongResults::deleteStatisticFilesAndData()
{
	statisticalValues.clear();
	binData.clear();
	QFile::remove(std::string(fileName + "/songbinIPI.txt").c_str());
	QFile::remove(std::string(fileName + "/songbinTrains.txt").c_str());
	QFile::remove(std::string(fileName + "/songbinCycles.txt").c_str());
	QFile::remove(std::string(fileName + "/songipi.txt").c_str());
	QFile::remove(std::string(fileName + "/songstatistics.txt").c_str());
	QFile::remove(std::string(fileName + "/songstatisticsoptions.txt").c_str());
	QFile::remove(std::string(fileName + "/songstatistics_done_success.txt").c_str());
}

SongResults::binStruct SongResults::readBinningFile(const char* filename)
{
	binStruct data;
	std::ifstream file (filename, std::ios::in);
	
	try{
		if(file.is_open() && file.good()){
			std::string line;
			std::string values;
			
			std::vector <std::string> rawdata;
			
			while(!file.eof()){
				getline(file, line);
				rawdata.push_back(line);
			}
			if(rawdata.size() == 2){
				QStringList headList = QString(rawdata[0].c_str()).split('\t');
				QStringList valueList = QString(rawdata[1].c_str()).split('\t');
				data.reserve(headList.size());
				for(size_t i = 0; headList.size() == valueList.size() && i < headList.size(); ++i){
					QString temp = headList[i].remove(0, 1);
					temp = temp.remove(temp.length()-1, 1);
					QStringList headPair = temp.split(':');
					
					if(headPair.size() == 2){
						data.push_back(std::make_pair( std::make_pair(headPair[0].toInt(), headPair[1].toInt()), valueList[i].toFloat()));
					}
				}
			}
		}
		return data;
	}catch(std::runtime_error &e){
		std::stringstream s;
		s << "Could not read file: \n" << filename << '\n';
		throw std::runtime_error(s.str());
	}
}

void SongResults::writeBinningFile(const binStruct& data, const char* filename)
{
	std::ofstream file(filename, std::ios::out);
	try{
		if(file.is_open() && file.good()){
			for(size_t i = 0; i < data.size(); ++i){
				file << "[" << data[i].first.first << ":" << data[i].first.second << ")";
				if(i != data.size()-1){
					file << '\t';
				}
			}
			file << '\n';
			for(size_t i = 0; i < data.size(); ++i){
				file << data[i].second;
				if(i != data.size()-1){
					file << '\t';
				}
			}
		}
	}catch(std::runtime_error &e){
		std::stringstream s;
		s << "Could not save file: \n" << filename << '\n';
		throw std::runtime_error(s.str());
	}
}

void SongResults::copyFile(const char* srcPath, const char* dstPath)
{
	std::ifstream src(srcPath);
	std::ofstream dst(dstPath);

	try{
		if(src.is_open() && src.good() && dst.is_open() && dst.good()){
			dst << src.rdbuf();
		}
	}catch(std::runtime_error &e){
		std::stringstream s;
		s << "Could not copy file: \n" << srcPath << '\n';
		throw std::runtime_error(s.str());
	}
}

// starts with the first given pulseStart index as biggest value. Iterating through the song until pulseEnd.
// if the current value is bigger than the old biggest value, the current value is set as biggest. if the values
// change from negative to positive values or vice versa, the current biggest value is saved as a peak (either
// positive or negative peak)
// if the end is reached it is tested if the current biggest value is the previous value. If so that means
// that the last "peak" was more or less a straight rising line and not counted as peak. After that it is tested
// if the positive peaks are higher than 1/3 of the highest positive peak and if the negative peaks are higher
// than 1/3 of the highest negative peak. The minimum of the remaining positive and negative peaks is returned
// as cycles per pulse
size_t SongResults::countPeakCycles(const Video& media, size_t pulseStart, size_t pulseEnd)
{
	const std::vector<float>& currentSong = media.getSong();

	std::vector<float> posPeaks;
	std::vector<float> negPeaks;
	if(pulseStart < 1){
		pulseStart = 0;
	}
	if(pulseEnd > currentSong.size()){
		pulseEnd = currentSong.size();
	}
	size_t biggest = pulseStart;
	int biggestPos = pulseStart;
	int biggestNeg = pulseStart;
	float prev = pulseStart;
	
	for(size_t i = pulseStart+1; i < pulseEnd; ++i){
		if(currentSong[i] > 0){ //if curr is positive
			if(currentSong[prev] > 0){ //if prev was positive
				if(currentSong[i] > currentSong[biggest]){ // and curr is bigger than prev
					biggest = i;
				}
			}else{			   //if prev was negative => comming back from neg. peak
				negPeaks.push_back(biggest); // save biggest as one neg. peak
				biggestNeg = currentSong[biggest] < currentSong[biggestNeg] ? biggest : biggestNeg;
				biggest = prev;
			}
		}else{
			if(currentSong[prev] < 0){ // same with negative
				if(currentSong[i] < currentSong[biggest]){
					biggest = i;
				}
			}else{
				posPeaks.push_back(biggest);
				biggestPos = currentSong[biggest] > currentSong[biggestPos] ? biggest : biggestPos;
				biggest = prev;
			}
		}
		prev = i;
	}
	if(biggest != prev){
		currentSong[biggest] > 0 ? posPeaks.push_back(biggest) : negPeaks.push_back(biggest);
	}

	size_t posCycles = 0;
	// if bigger than 1/3 of the biggest positive peak: count as peak
	for(size_t i = 0; i < posPeaks.size(); ++i){ 
		if(currentSong[posPeaks[i]] > (currentSong[biggestPos] / 3)){
			++posCycles;
		}
	}

	size_t negCycles = 0;
	// if bigger than 1/3 of the biggest negative peak: count as peak
	for(size_t i = 0; i < negPeaks.size(); ++i){
		if(currentSong[negPeaks[i]] < (currentSong[biggestNeg] / 3)){
			++negCycles;
		}
	}

	return posCycles > negCycles ? negCycles : posCycles;
}
