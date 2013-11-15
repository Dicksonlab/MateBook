#ifndef SongResults_hpp
#define SongResults_hpp

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <QVector4D>
#include <QString>
#include <QStringList>
#include "../../common/source/serialization.hpp"

class Video;

class SongResults {
public:
	typedef std::vector<std::pair< std::pair<size_t, size_t>, float> > binStruct;

	SongResults();	//TODO: is this constructor needed?
	SongResults(const std::string& filepath);
	~SongResults();

	std::string getFileName() const;

	bool createPulse(const Video& media, std::pair<int, int> indexRange);
	bool deletePulses(std::pair<int, int> indexRange);

	bool createSine(const Video& media, std::pair<int, int> indexRange);
	bool deleteSines(std::pair<int, int> indexRange);

	const std::map<size_t, size_t>& getPulses() const; // in samples
	const std::map<size_t, size_t>& getSines() const; // in samples
	const std::map<size_t, size_t>& getTrains() const; // train start: first pulse start, train end: last pulse start
	const std::map<size_t, size_t>& getIPI() const; // pulseStart, distance in samples
	const std::map<size_t, size_t>& getPulseCycles() const; // pulseStart, number of cycles
	const std::map<size_t, size_t>& getPulseCenters() const; // pulseStart, position
	float getStatisticalValue(std::string name);
	binStruct getBinData(std::string name);

	void setFilePath(const std::string& fileName);
	void setStatisticalValue(std::string name, float value);
	void setBinData(std::string name, const binStruct& data);

	void writeStatisticFile();
	void writeFileCleanedFile();
	void writePulseDetectionOptions(const std::map<QString, float> &options);
	void writeSongStatisticsOptions(const std::map<QString, float> &options);
	
	void readStatisticFile();
	std::map<QString, float> readPulseDetectionOptions();
	std::map<QString, float> readSongStatisticsOptions();

	void reloadData();
	void reloadRawData();
	void reloadCleanData();
	void saveData();
	void saveCleanData();
	void clearAllData();

	void calculateStatisticalValues(std::map<QString, float> statisticOptions, size_t sampleRate, float samples, size_t startSample, size_t endSample);

private:

	std::string fileName;
	std::map<size_t, size_t> pulseCenters;
	std::map<size_t, size_t> pulseCycles;
	std::map<size_t, size_t> ipi;
	std::map<size_t, size_t> pulses;
	std::map<size_t, size_t> sines;
	std::map<size_t, size_t> trains;
	std::map<std::string, float> statisticalValues; // standardDeviation, mean etc. (name, value)
	std::map<std::string, binStruct> binData;

	int pulsePerMinuteCount;
	int pulsePerMinuteAllCount;

	template<typename K, typename V>
	std::map<K, V> readPairFile(const char* filename){
		std::map<K, V> pmap;
		std::ifstream file (filename, std::ios::in);
		std::string line;
		int linecount=0;
		if(file.is_open()){
			while(file.good()){
				getline(file, line);
				++linecount; // just for error message
				if(!file.eof()){
					QString temp(line.c_str());
					QStringList list = temp.split('\t');
					if(list.size() == 2){
						try{
							pmap[unstringify<K>(list[0].toStdString())] = unstringify<V>(list[1].toStdString());
						}catch(std::runtime_error &e){
							std::stringstream s;
							s << e.what() << " at line: " << linecount << " in file: " << filename;
							throw std::runtime_error(s.str());
						}
					}else{
						std::stringstream s;
						s << "Wrong file format. Couldn't read data\nin file: " << filename << " at line: " << linecount;
						throw std::runtime_error(s.str());
					}
				}
			}
			file.close();
		}
		return pmap;
	}

	template <typename K, typename V>
	void writePairFile(const std::map<K, V>& data, const char* filename){
		std::ofstream file(filename, std::ios::out);
		try{
			if(file.is_open() && file.good()){
				for(typename std::map<K, V>::const_iterator it = data.begin(); it != data.end(); ++it){
					file << (*it).first << '\t' << (*it).second << '\n';
				}
			}
		}catch(std::runtime_error &e){
			std::stringstream s;
			s << "Could not save file: \n" << filename << '\n';
			throw std::runtime_error(s.str());
		}
	}

	void detectTrains(size_t minTTrainLength, size_t minITrainLength, size_t minCTrainLength, size_t pulsePerMinuteExclude, size_t maxIPIdistance, size_t minIPIdistance, size_t startSample, size_t endSample);
	void deleteStatisticFilesAndData();
	binStruct readBinningFile(const char* filename);
	void writeBinningFile(const binStruct& data, const char* filename);

	void copyFile(const char* srcPath, const char* dstPath);

	size_t countPeakCycles(const Video& media, size_t pulseStart, size_t pulseEnd);
};

#endif
