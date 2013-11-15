#include "SongVisitor.hpp"
#include <QFileInfo>
#include <QString>
#include <iostream>
#include <numeric>
#include <cmath>
#include "FileItem.hpp"
#include "RuntimeError.hpp"
#include "SongResults.hpp"

int SongVisitor::visit(const Item* item)
{
	if (visitedItems.find(item) != visitedItems.end()) {
		return 0;
	}
	visitedItems.insert(item);

	if (item->getCurrentAudioStage() > Item::AudioRecording && item->getCurrentAudioStatus() == Item::Finished){
		if (const FileItem* const fileItem = dynamic_cast<const FileItem*>(item)){
			SongResults results = fileItem->getSongResults();
			results.reloadData();
			std::map<QString, float> options = results.readSongStatisticsOptions();
			SongFile song;

			song.fileName = fileItem->getFileName().toStdString();
			song.pulsedetectOptions = results.readPulseDetectionOptions();
			song.statisticalOptions = options;
			song.duration = item->getEndTime() - item->getStartTime();

			song.bins["ipiBin"] = results.getBinData("binIPI");
			song.bins["cycleBin"] = results.getBinData("binCycles");
			song.bins["trainBin"] = results.getBinData("binTrains");
			
			song.pulseCount = fileItem->getPulses();
			song.ipiCount = results.getIPI().size();
			song.trainCount = results.getTrains().size();
			// check if the File Item has a certain number of trains, pulses and ipis
			if(results.getPulseCycles().size() >= options["Batch:MinPulseNumber"]){
				song.pulsesPerMinute = results.getStatisticalValue("pulsesPerMinute");
				song.pulsesPerMinuteExclude = results.getStatisticalValue("pulsesPerMinuteExclude");
				song.meanCPP = results.getStatisticalValue("meanCPP");
				song.standardDeviationCPP = results.getStatisticalValue("standardDeviationCycle");
				song.standardErrorCPP = results.getStatisticalValue("standardErrorCycle");
			}else{
				song.pulsesPerMinute = -1;
				song.pulsesPerMinuteExclude = -1;
				song.meanCPP = -1;
				song.standardDeviationCPP = -1;
				song.standardErrorCPP = -1;
			}

			if(results.getIPI().size() >= options["Batch:MinIPInumber"]){
				song.meanIPI = results.getStatisticalValue("meanIPI");
				song.standardDeviationIPI = results.getStatisticalValue("standardDeviationIPI");
				song.standardErrorIPI = results.getStatisticalValue("standardErrorIPI");
			}else{
				song.meanIPI = -1;
				song.standardDeviationIPI = -1;
				song.standardErrorIPI = -1;
			}

			if(results.getTrains().size() >= options["Batch:MinTrainNumber"]){
				song.meanTrain = results.getStatisticalValue("meanTrain");
				song.standardDeviationTrain = results.getStatisticalValue("standardDeviationTrain");
				song.standardErrorTrain = results.getStatisticalValue("standardErrorTrain");
			}else{
				song.meanTrain = -1;
				song.standardDeviationTrain = -1;
				song.standardErrorTrain = -1;
			}

			song.meanSineDuration = results.getStatisticalValue("meanSineDuration");
			song.totalSineDuration = results.getStatisticalValue("totalSineDuration");
			song.sineEpisodeCount = results.getStatisticalValue("sineEpisodeCount");

			song.experimenter = item->getExperimenter();
			song.sex1 = item->getFirstSex();
			song.genotype1 = item->getFirstGenotype();
			song.sex2 = item->getSecondSex();
			song.genotype2 = item->getSecondGenotype();
			song.comment = item->getComment();

			songFiles.push_back(song);
		}
	}
	return songFiles.size();
}

void SongVisitor::writeReport(const std::string& fileName)
{
	try{
		std::ofstream reportFile(fileName.c_str());
		if(reportFile.is_open()){
				std::map<QString, float> detOptions; 
				std::map<QString, float> staOptions;
				int i = 0;
				while(songFiles.size() > i && detOptions.empty() && staOptions.empty()){
					detOptions = songFiles[i].pulsedetectOptions;
					staOptions = songFiles[i].statisticalOptions;
					++i;
				}

				float sumMeanIPI = 0;
				float sumMeanCPP = 0;
				float sumMeanTrain = 0;
				float sumStandardDeviationIPI = 0;
				float sumStandardDeviationCPP = 0;
				float sumStandardDeviationTrain = 0;
				float sumStandardErrorIPI = 0;
				float sumStandardErrorCPP = 0;
				float sumStandardErrorTrain = 0;
				float sumPulsePerMinute = 0;
				float sumPulsePerMinuteExclude = 0;
				int sumIPI = 0;
				float sumMeanSineDuration = 0;
				float sumTotalSineDuration = 0;

				size_t ipiFiles = songFiles.size();
				size_t trainFiles = songFiles.size();
				size_t cppFiles = songFiles.size();
				size_t pulsePMin = songFiles.size();
				size_t pulsePMinExc = songFiles.size();
				size_t sineFiles = songFiles.size();

				const char separator = '\t';
				reportFile <<
					"File" << separator <<
					"Duration (sec)" << separator <<
					"Pulses (#)" << separator <<
					"Pulses/min" << separator <<
					"Pulses/min excluding pulses" << separator <<
					"CPP mean" << separator <<
					"CPP standard deviation" << separator <<
					"CPP standard error" << separator <<
					"CPP mode" << separator <<
					"IPI count" << separator <<
					"IPI mean (ms)" << separator <<
					"IPI standard deviation" << separator <<
					"IPI standard error" << separator <<
					"IPI mode (ms)" << separator <<
					"Train count" << separator <<
					"PPT mean" << separator <<	// pulses per train
					"PPT standard deviation" << separator <<
					"PPT standard error" << separator <<
					"PPT mode" << separator <<
					"Sine song episodes (#)" << separator <<
					"Sine song duration mean (ms)" << separator <<
					"Sine song duration total (ms)" << separator <<
					"Experimenter" << separator <<
					"1st sex" << separator <<
					"1st genotype" << separator <<
					"2nd sex" << separator <<
					"2nd genotype" << separator <<
					"Comment" << '\n'
				;

				std::vector<SongResults::binStruct> ipiBins;
				std::vector<SongResults::binStruct> trainsBins;
				
				for(int i = 0; i < songFiles.size(); ++i){
					SongResults::binStruct ipiBin = songFiles[i].bins["ipiBin"];
					SongResults::binStruct cyclesBin = songFiles[i].bins["cycleBin"];
					SongResults::binStruct trainsBin = songFiles[i].bins["trainBin"];
					
					if(ipiBin.size()>0){
						ipiBins.push_back(ipiBin);
					}
					if(trainsBin.size()>0){
						trainsBins.push_back(trainsBin);
					}

					size_t modalIPIindex = 0;
					for(int j = 0; j < ipiBin.size(); ++j){ // get index for the biggest element this vector
						modalIPIindex = ipiBin[modalIPIindex].second < ipiBin[j].second? j : modalIPIindex;
					}
					size_t modalCycleIndex = 0;
					for(int j = 0; j < cyclesBin.size(); ++j){ // get index for the biggest element this vector
						modalCycleIndex = cyclesBin[modalCycleIndex].second < cyclesBin[j].second? j : modalCycleIndex;
					}
					size_t modalTrainIndex = 0;
					for(int j = 0; j < trainsBin.size(); ++j){ // get index for the biggest element this vector
						modalTrainIndex = trainsBin[modalTrainIndex].second < trainsBin[j].second? j : modalTrainIndex;
					}

					// save the upper limits of the bins with the highest number of elements (modal bin)
					float iBin = modalIPIindex < ipiBin.size()? ipiBin[modalIPIindex].first.second : 0;
					float cBin = modalCycleIndex < cyclesBin.size()? cyclesBin[modalCycleIndex].first.second : 0;
					float tBin = modalTrainIndex < trainsBin.size()? trainsBin[modalTrainIndex].first.second : 0;

					QFileInfo audioFile(QString::fromStdString(songFiles[i].fileName));
					reportFile << audioFile.fileName().toStdString() << "\t"
							   << songFiles[i].duration << "\t";

					songFiles[i].pulseCount > -1? reportFile << songFiles[i].pulseCount : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].pulsesPerMinute > -1? reportFile << songFiles[i].pulsesPerMinute : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].pulsesPerMinute > -1? reportFile << songFiles[i].pulsesPerMinuteExclude : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].meanCPP > -1? reportFile << songFiles[i].meanCPP : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].standardDeviationCPP > -1? reportFile << songFiles[i].standardDeviationCPP : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].standardErrorCPP > -1? reportFile << songFiles[i].standardErrorCPP : reportFile << "N.A.";
					reportFile << '\t';
				    cBin > 0? reportFile << cBin : reportFile << "N.A.";
					reportFile << '\t';
				    songFiles[i].ipiCount > -1? reportFile << songFiles[i].ipiCount : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].meanIPI > -1? reportFile << songFiles[i].meanIPI : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].standardDeviationIPI > -1? reportFile << songFiles[i].standardDeviationIPI : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].standardErrorIPI > -1? reportFile << songFiles[i].standardErrorIPI : reportFile << "N.A.";
					reportFile << '\t';
				    iBin > 0? reportFile << iBin : reportFile << "N.A.";
					reportFile << '\t';
				    songFiles[i].trainCount > -1? reportFile << songFiles[i].trainCount : reportFile << "N.A.";
					reportFile << '\t';
				    songFiles[i].meanTrain > -1? reportFile << songFiles[i].meanTrain : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].standardDeviationTrain > -1? reportFile << songFiles[i].standardDeviationTrain : reportFile << "N.A.";
					reportFile << '\t';
					songFiles[i].standardErrorTrain > -1? reportFile << songFiles[i].standardErrorTrain : reportFile << "N.A.";
					reportFile << '\t';
				    tBin > 0? reportFile << tBin : reportFile << "N.A.";
					reportFile << '\t';
					reportFile << songFiles[i].sineEpisodeCount;
					reportFile << '\t';
					songFiles[i].meanSineDuration > -1? reportFile << songFiles[i].meanSineDuration : reportFile << "N.A.";
					reportFile << '\t';
					reportFile << songFiles[i].totalSineDuration;
					reportFile << '\t';
					reportFile << songFiles[i].experimenter << '\t';
					reportFile << songFiles[i].sex1 << '\t';
					reportFile << songFiles[i].genotype1 << '\t';
					reportFile << songFiles[i].sex2 << '\t';
					reportFile << songFiles[i].genotype2 << '\t';
					reportFile << songFiles[i].comment << '\t';
					reportFile << '\n';

					// create total sum for pulses per minute
					if(songFiles[i].pulsesPerMinute > -1){
						sumPulsePerMinute += songFiles[i].pulsesPerMinute;
					}else{
						--pulsePMin;
					}

					if(songFiles[i].pulsesPerMinuteExclude > -1){
						sumPulsePerMinuteExclude += songFiles[i].pulsesPerMinuteExclude;
					}else{
						--pulsePMinExc;
					}

					// create total sum for mean, standard deviation and standard error
					if(songFiles[i].meanIPI > -1){
						sumMeanIPI += songFiles[i].meanIPI;
						sumStandardDeviationIPI += songFiles[i].standardDeviationIPI;
						sumStandardErrorIPI += songFiles[i].standardErrorIPI;
						sumIPI += songFiles[i].ipiCount;
					}else{
						--ipiFiles;
					}

					if(songFiles[i].meanTrain > -1){
						sumMeanTrain += songFiles[i].meanTrain;
						sumStandardDeviationTrain += songFiles[i].standardDeviationTrain;
						sumStandardErrorTrain += songFiles[i].standardErrorTrain;
					}else{
						--trainFiles;
					}

					if(songFiles[i].meanCPP > -1){
						sumMeanCPP += songFiles[i].meanCPP;
						sumStandardDeviationCPP += songFiles[i].standardDeviationCPP;
						sumStandardErrorCPP += songFiles[i].standardErrorCPP;
					}else{
						--cppFiles;
					}	

					sumTotalSineDuration += songFiles[i].totalSineDuration;
					if (songFiles[i].meanSineDuration > -1 ) {
						sumMeanSineDuration += songFiles[i].meanSineDuration;
					} else {
						--sineFiles;
					}	
				}

				std::string binIPIfile = fileName + ".ipi";
				std::string binTrainFile = fileName + ".train";

				std::ofstream binningDataIPI(binIPIfile.c_str());
				std::ofstream binningDataTrain(binTrainFile.c_str());

				std::vector<float> meanIPIBins;
				std::vector<float> errorsIPI;
				size_t modalIPIindex = 0;
				size_t mostBinElements = 0;

				//============ write bar chart data
				if(binningDataIPI.is_open() && binningDataTrain.is_open() && songFiles.size() > 0){
					size_t index = 0;
					for(size_t j = 0; j < ipiBins.size(); ++j){ // get index for element with largest size
						if(ipiBins[index].size() < ipiBins[j].size()){
							index = j;
						}
					}
					binningDataIPI << '\t';
					for(size_t j = 0; ipiBins.size() > 0 && j < ipiBins[index].size(); ++j){ // create headlines for bin percentages
						binningDataIPI << "[" << ipiBins[index].at(j).first.first << ":" << ipiBins[index].at(j).first.second << ")";
						if(j < ipiBins[index].size()-1){
							binningDataIPI << '\t';
						}
					}
					binningDataIPI << '\n';

					calculateBinStatistics(ipiBins, meanIPIBins, errorsIPI);
					writeStatisticalBinningValues(binningDataIPI, meanIPIBins, "means");
					writeStatisticalBinningValues(binningDataIPI, errorsIPI, "errors");
					modalIPIindex = std::distance(meanIPIBins.begin(), std::max_element(meanIPIBins.begin(), meanIPIBins.end()));
					mostBinElements = index;

					std::vector<float> meanTrainBins;
					std::vector<float> errorsTrain;

					index = 0;
					for(size_t j = 0; j < trainsBins.size(); ++j){ // // get index for element with largest size
						if(trainsBins[index].size() < trainsBins[j].size()){
							index = j;
						}
					}
					binningDataTrain << '\t';
					for(size_t j = 0; trainsBins.size() > 0 && j < trainsBins[index].size(); ++j){
						binningDataTrain << "[" << trainsBins[index].at(j).first.first << ":" << trainsBins[index].at(j).first.second << ")";
						if(j < trainsBins[index].size()-1){
							binningDataTrain << '\t';
						}
					}
					binningDataTrain << '\n';

					calculateBinStatistics(trainsBins, meanTrainBins, errorsTrain);
					writeStatisticalBinningValues(binningDataTrain, meanTrainBins, "means");
					writeStatisticalBinningValues(binningDataTrain, errorsTrain, "errors");
				}

				binningDataIPI.close();
				binningDataTrain.close();

				reportFile << "\n\nNr of Files analysed:\t" << songFiles.size() << '\n';
				reportFile << "Analysed IPI Sum:\t" << sumIPI << '\n';
				reportFile << "Modal IPI (ms):\t" << "[";
				ipiBins.size() > mostBinElements && ipiBins[mostBinElements].size() > modalIPIindex? reportFile << ipiBins[mostBinElements].at(modalIPIindex).first.first : reportFile << "-";
				reportFile << ":";
				ipiBins.size() > mostBinElements && ipiBins[mostBinElements].size() > modalIPIindex? reportFile << ipiBins[mostBinElements].at(modalIPIindex).first.second : reportFile << "-";
				reportFile << ")\t";
				meanIPIBins.size() > modalIPIindex? reportFile << meanIPIBins[modalIPIindex] : reportFile << "N.A.";
				reportFile << " % \n";
				
				reportFile << "Mean Pulses per Minutes:\t";
				pulsePMin > 0? reportFile << sumPulsePerMinute / pulsePMin << '\t' << pulsePMin << " files used\n" : reportFile << "N.A.\n";
				reportFile << "Mean Pulses per Minutes (excluding pulses):\t";
				pulsePMinExc > 0? reportFile << sumPulsePerMinuteExclude / pulsePMinExc << '\t' << pulsePMinExc << " files used\n\n" : reportFile << "N.A.\n\n";

				if(ipiFiles > 0){
					reportFile << "Mean Mean IPI (ms):  \t" << sumMeanIPI/ipiFiles << '\t' << ipiFiles << " files used\n";
					reportFile << "Mean Standard Deviation IPI (ms):\t" << sumStandardDeviationIPI/ipiFiles << '\n';
					reportFile << "Mean Standard Error IPI (ms):\t" << sumStandardErrorIPI/ipiFiles << "\n\n";
				}else{
					reportFile << "Mean Mean IPI, Mean Standard Deviation IPI and Mean Standard Error IPI are N.A.\n\n";
				}

				if(trainFiles > 0){
					reportFile << "Mean Mean Trains (pulses):  \t" << sumMeanTrain/trainFiles << '\t' << trainFiles << " files used\n";
					reportFile << "Mean Standard Deviation Trains (pulses):\t" << sumStandardDeviationTrain/trainFiles << '\n';
					reportFile << "Mean Standard Error Trains (pulses):\t" << sumStandardErrorTrain/trainFiles << "\n\n";
				}else{
					reportFile << "Mean Mean Trains, Mean Standard Deviation Trains and Mean Standard Error Trains are N.A.\n\n";
				}

				if(cppFiles > 0){
					reportFile << "Mean Mean CPP (#):  \t" << sumMeanCPP/cppFiles << '\t' << cppFiles << " files used\n";
					reportFile << "Mean Standard Deviation CPP (#):\t" << sumStandardDeviationCPP/cppFiles << '\n';
					reportFile << "Mean Standard Error CPP (#):\t" << sumStandardErrorCPP/cppFiles << "\n\n";
				}else{
					reportFile << "Mean Mean CPP, Mean Standard Deviation CPP and Mean Standard Error CPP are N.A.\n\n";
				}

				if (songFiles.size()) {
					reportFile << "Mean Total Sine Song Episode Duration (ms):\t" << sumTotalSineDuration / songFiles.size() << '\n';
				} else {
					reportFile << "Mean Total Sine Song Episode Duration is N.A.\n\n";
				}
				if (sineFiles > 0) {
					reportFile << "Mean Mean Sine Song Episode Duration (ms):\t" << sumMeanSineDuration / sineFiles << '\t' << sineFiles << " files used\n";
				} else {
					reportFile << "Mean Mean Sine Song Episode Duration is N.A.\n\n";
				}

				reportFile << "\nSong Statistics Options:\n";
				for(std::map<QString, float>::iterator iter = staOptions.begin(); iter != staOptions.end(); ++iter){
					reportFile << (*iter).first.toStdString() << ":\t" << (*iter).second << "\n";
				}
				
				reportFile << "\n\nSong Analysis Options:\n";
				for(std::map<QString, float>::iterator iter = detOptions.begin(); iter != detOptions.end(); ++iter){
					reportFile << (*iter).first.toStdString() << ":\t" << (*iter).second << "\n";
				}
		}
		reportFile.close();
	}catch(std::runtime_error &e){
		std::stringstream s;
		s << "Could not read file: \n" << fileName << '\n';
		throw std::runtime_error(s.str());
	}
}

//file structur: name, value1, value2, value3 ...
//				 error, value1, value2, value3 ...
//				 name, value1, value2, value3 ...
//				 error, value1, value2, value3 ...
//				 ...
void SongVisitor::writeStatisticalBinningValues(std::ofstream& stream, const std::vector<float>& data, std::string name)
{
	stream << name << '\t';
	for(size_t j = 0; j < data.size(); ++j){
		stream << data[j];
		if(j == data.size()-1){
			stream << '\n';
		}else{
			stream << '\t';
		}
	}
}

// Calculate the mean for each bin
// bin1: 20-25 ms  Song1: 20%, Song 2: 15% Song3: 25%  mean: 20%+/-xx%
// bin2: 25-30 ms Song1: 50%, Song 2: xx% Song3: xx%  mean: xx%+/-xx%
// bin3: 30-35 ms Song1: 30%, Song 2: xx% Song3: xx%  mean: xx%+/-xx%
void SongVisitor::calculateBinStatistics(const std::vector<SongResults::binStruct>& source, std::vector<float>& mean, std::vector<float>& error)
{
	if(source.size() > 0){
		int j = 0;
		int biggest = 1;
		while(j < biggest){
			float subtotal = 0;
			for(size_t i = 0; i < source.size(); ++i){
				biggest = biggest < source[i].size()? source[i].size() : biggest;
				if(j < source[i].size()){
					subtotal += source[i].at(j).second;
				}
			}
			mean.push_back(subtotal/source.size());
			++j;
		}

		j = 0;
		while(j < biggest){
			float subtotal = 0;
			for(size_t i = 0; i < source.size(); ++i){
				if(j < source[i].size()){
					float diff = source[i].at(j).second - mean[j];
					subtotal += diff * diff;
				}
			}
			error.push_back( sqrt(subtotal/source.size()) / sqrt((float)source.size()) );
			++j;
		}
	}
}