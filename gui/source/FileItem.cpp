#include "FileItem.hpp"
#include <QStringList>
#include <QFileInfo>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include "RuntimeError.hpp"
#include "../../common/source/Singleton.hpp"
#include "global.hpp"
#include "ExternalJob.hpp"
#include "ClusterJob.hpp"
#include "JobQueue.hpp"
#include "ArenaItem.hpp"
#include "MateBook.hpp"
#include "Project.hpp"
#include "FileUtilities.hpp"
#include "Version.hpp"
#include "ConfigDialog.hpp"
#include "../../common/source/serialization.hpp"
#include "../../common/source/system.hpp"

FileItem::FileItem(MateBook* mateBook, Project* project, const QDir& relativeDataDirectory, FileItem* parent) : Item(mateBook, project),
	parentItem(parent),
	dataDirectory(relativeDataDirectory),
	video(),
	currentVideoStage(VideoRecording),
	currentVideoStatus(Finished),
	currentAudioStage(AudioRecording),
	currentAudioStatus(Finished),
	startTime(),
	endTime(),
	fileDate(),
	samples(),
	sampleRate(),
	pulses(),
	sines(),
	trains(),
	pulseCenters(),
	songOptionId()
{
	registerSettings();

	// get the path of the video and other meta information
	QString videoMetaFileName("video.tsv");
	try {
		videoInfo.importFrom(absoluteDataDirectory().filePath(videoMetaFileName).toStdString());
	} catch (const std::bad_cast& e) {
		throw RuntimeError(QObject::tr("Could not parse %1.").arg(absoluteDataDirectory().filePath(videoMetaFileName)));
	}

	updateStateFromFiles();
}

FileItem::FileItem(MateBook* mateBook, Project* project, const QString& fileName, FileItem* parent) : Item(mateBook, project),
	parentItem(parent),
	video(),
	videoFilePath(),
	width(),
	height(),
	numFrames(),
	fps(),
	currentVideoStage(VideoRecording),
	currentVideoStatus(Finished),
	currentAudioStage(AudioRecording),
	currentAudioStatus(Finished),
	startTime(),
	endTime(),
	fileDate(),
	noiseFileName(),
	samples(),
	sampleRate(),
	pulses(),
	sines(),
	trains(),
	pulseCenters()
{
	registerSettings();

	// create a new (!) subdirectory of the project directory
	QString videoFileName = QFileInfo(fileName).fileName();
	QString directoryName;
	unsigned int tries = 10;
	do {
		if (tries-- == 0) {
			throw RuntimeError(QObject::tr("Could not create a result directory for video %1.").arg(videoFileName));
		}
		directoryName = videoFileName + "." + QString::number(qrand()) + QString(".mbr");
	} while (!getProject()->getDirectory().mkdir(directoryName));
	dataDirectory = QDir(directoryName);

	boost::shared_ptr<Video> videoSharedPtr(new Video(fileName.toStdString()));
	video = videoSharedPtr;
	videoFilePath = videoSharedPtr->getFileName();
	width = videoSharedPtr->getWidth();
	height = videoSharedPtr->getHeight();
	numFrames = videoSharedPtr->getNumFrames();
	fps = videoSharedPtr->getFps();

	noiseFileName = "";
	samples = videoSharedPtr->getSamples();
	sampleRate = videoSharedPtr->getSampleRate();

	endTime = videoSharedPtr->isVideo()? numFrames / fps : samples / sampleRate;

	fileDate = QFileInfo(QString::fromStdString(videoSharedPtr->getFileName())).created();

	try {
		experimenter = getUserName();
	} catch (const std::runtime_error&) {
		// cannot get the username so we just leave the field empty
	}

	serialize();
}

FileItem::~FileItem()
{
	for (int i = 0; i < childItems.size(); ++i) {
		delete childItems[i];
	}
	childItems.clear();
}

void FileItem::appendChild(ArenaItem* child)
{
	emit beginInsertChildren(this, childItems.size(), childItems.size());
	childItems.push_back(child);
	emit endInsertChildren(this, childItems.size() - 1, childItems.size() - 1);
	connect(childItems.back(), SIGNAL(itemChanged(ArenaItem*)), this, SLOT(childItemChanged(ArenaItem*)));
}

Item* FileItem::child(int row) const
{
	return childItems[row];
}

int FileItem::childCount() const
{
	return childItems.size();
}

FileItem* FileItem::parent()
{
	return parentItem;
}

void FileItem::removeChildren(int position, int count)
{
	if (count < 1) {
		return;
	}

	emit beginRemoveChildren(this, position, position + count - 1);
	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin() + position; iter != childItems.begin() + position + count; ++iter) {
		(*iter)->removeData();
		delete *iter;
	}
	childItems.erase(childItems.begin() + position, childItems.begin() + position + count);
	emit endRemoveChildren(this, position, position + count - 1);
}

//TODO: almost a duplicate of the above
void FileItem::removeChildrenKeepData(int position, int count)
{
	if (count < 1) {
		return;
	}

	emit beginRemoveChildren(this, position, position + count - 1);
	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin() + position; iter != childItems.begin() + position + count; ++iter) {
		delete *iter;
	}
	childItems.erase(childItems.begin() + position, childItems.begin() + position + count);
	emit endRemoveChildren(this, position, position + count - 1);
}

int FileItem::childIndex() const
{
	assert(parentItem);	// precondition violated
	//TODO: change inefficient linear search?
	for (int index = 0; index < parentItem->childCount(); ++index) {
		if (parentItem->child(index) == this) {
			return index;
		}
	}
	assert(false);	// child not in the parent's list
	return -1;
}

void FileItem::removeData()
{
	removeChildren(0, childCount());
	try {
		FileUtilities::removeDirectory(absoluteDataDirectory());
	} catch (RuntimeError& e) {
		std::cerr << "FileItem::removeData: " << e.what() << std::endl;
	}
}

boost::shared_ptr<Video> FileItem::getVideo() const
{
	if (boost::shared_ptr<Video> ret = video.lock()) {
		return ret;
	}
	boost::shared_ptr<Video> ret(new Video(videoFilePath));
	video = ret;
	return ret;
}

SongResults FileItem::getSongResults() const
{
	SongResults songResults(absoluteDataDirectory().absolutePath().toStdString());
	return songResults;
}

QString FileItem::getFileName() const
{
	return QString::fromStdString(videoFilePath);
}

void FileItem::setFileName(const QString& name)
{
	videoFilePath = name.toStdString();
	emit itemChanged(this);
}

std::string FileItem::getId() const
{
	return absoluteDataDirectory().dirName().toStdString();
}

unsigned int FileItem::getWidth() const
{
	return width;
}

unsigned int FileItem::getHeight() const
{
	return height;
}

unsigned int FileItem::getNumFrames() const
{
	return numFrames;
}

double FileItem::getFps() const
{
	return fps;
}

QString FileItem::getNoiseFileName() const
{
	return QString::fromStdString(noiseFileName);
}

void FileItem::setNoiseFileName(const QString& name)
{
	noiseFileName = name.toStdString();
}

unsigned int FileItem::getSamples() const
{
	return samples;
}

unsigned int FileItem::getSampleRate() const
{
	return sampleRate;
}

unsigned int FileItem::getPulses() const 
{
	return pulses;	
}

unsigned int FileItem::getSines() const
{
	return sines;
}

unsigned int FileItem::getTrains() const
{
	return trains;
}

unsigned int FileItem::getPulseCenters() const
{
	return pulseCenters;
}

QString FileItem::getSongOptionId() const
{
	return QString::fromStdString(songOptionId);
}

// loads the statistical options from a file and returns them
std::map<QString, float> FileItem::getPulseDetectionOptions()
{
	std::map<QString, float> pulseDetectionOptions = getSongResults().readPulseDetectionOptions();
	return pulseDetectionOptions;
}

// loads the statistical options from a file and returns them
std::map<QString, float> FileItem::getSongStatisticsOptions()
{
	std::map<QString, float> statisticsOptions = getSongResults().readSongStatisticsOptions();
	return statisticsOptions;
}

unsigned int FileItem::getDuration() const
{
	return samples==0? numFrames / fps : samples / sampleRate; //TODO: use other possibilities
}

QDateTime FileItem::getDate() const
{
	return fileDate;
}

void FileItem::setDate(const QDateTime& date)
{
	fileDate = date;
}

std::string FileItem::getFirstSex() const
{
	return firstSex;
}

void FileItem::setFirstSex(const std::string& firstSex)
{
	this->firstSex = firstSex;
}

std::string FileItem::getFirstGenotype() const
{
	return firstGenotype;
}

void FileItem::setFirstGenotype(const std::string& firstGenotype)
{
	this->firstGenotype = firstGenotype;
}

std::string FileItem::getSecondSex() const
{
	return secondSex;
}

void FileItem::setSecondSex(const std::string& secondSex)
{
	this->secondSex = secondSex;
}

std::string FileItem::getSecondGenotype() const
{
	return secondGenotype;
}

void FileItem::setSecondGenotype(const std::string& secondGenotype)
{
	this->secondGenotype = secondGenotype;
}

unsigned int FileItem::getStartTime() const
{
	return startTime;
}

void FileItem::setStartTime(const unsigned int time)
{
	startTime = std::min(time, endTime);
	for (int i = 0; i < childItems.size(); ++i) {
		childItems[i]->setStartTime(startTime);
	}
}

unsigned int FileItem::getEndTime() const
{
	return endTime;
}

void FileItem::setEndTime(const unsigned int time)
{
	endTime = std::min(time, getDuration());
	endTime = std::max(startTime, endTime);
	for (int i = 0; i < childItems.size(); ++i) {
		childItems[i]->setEndTime(endTime);
	}
}

std::string FileItem::getExperimenter() const
{
	return experimenter;
}

void FileItem::setExperimenter(const std::string& experimenter)
{
	this->experimenter = experimenter;
}

unsigned int FileItem::getApproved() const
{
	unsigned int count = 0;
	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		count += (*iter)->getApproved();
	}
	return count;
}

void FileItem::setApproved(bool approved)
{
	for (int i = 0; i < childItems.size(); ++i) {
		childItems[i]->setApproved(approved);
	}
}

std::string FileItem::getComment() const
{
	return comment;
}

void FileItem::setComment(const std::string& comment)
{
	this->comment = comment;
}

bool FileItem::hasCourtship() const
{
	bool any = false;
	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		any = any || (*iter)->hasCourtship();
	}
	return any;
}

double FileItem::getCourtship() const
{
	unsigned int count = 0;
	double sum = 0;
	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		if ((*iter)->hasCourtship()) {
			++count;
			sum += (*iter)->getCourtship();
		}
	}
	return (count == 0) ? 0 : sum / static_cast<double>(count);
}

bool FileItem::hasQuality() const
{
	bool any = false;
	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		any = any || (*iter)->hasQuality();
	}
	return any;
}

double FileItem::getQuality() const
{
	double min = 1;
	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		if ((*iter)->hasQuality()) {
			min = std::min(min, (*iter)->getQuality());
		}
	}
	return min;
}

void FileItem::serialize() const
{
	Item::serialize();
	QString videoMetaFileName("video.tsv");
	videoInfo.exportTo(absoluteDataDirectory().filePath(videoMetaFileName).toStdString());

	//TODO: write X_arena.csv and X_mask.bmp (if the user has added / removed arenas)

	for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		(*iter)->serialize();
	}
	// call .serialize on other members here if necessary
}

QDir FileItem::relativeDataDirectory() const
{
	return dataDirectory;
}

QDir FileItem::absoluteDataDirectory() const
{
	QString absoluteProjectDirectoryString = getProject()->getDirectory().canonicalPath();
	QString relativeDataDirectoryString = dataDirectory.path();
	return absoluteProjectDirectoryString + "/" + relativeDataDirectoryString;
}

void FileItem::runArenaDetection()
{
	if (currentVideoStage < ArenaDetection) {
		QString settingsFileName = "settings_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv";
		getMateBook()->writeTrackerSettings(absoluteDataDirectory().path() + "/" + settingsFileName);
		Job* jobToQueue = createJob(settingsFileName, true, false, false);
		//TODO: remove any existing preprocessor output files
		currentVideoStage = ArenaDetection;
		currentVideoStatus = Queued;
		emit itemChanged(this);
		queueJob(jobToQueue);
	}
}

void FileItem::runFlyTracking()
{
	Job* jobToQueue = NULL;
	if (currentVideoStage < ArenaDetection) {
		QString settingsFileName = "settings_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv";
		getMateBook()->writeTrackerSettings(absoluteDataDirectory().path() + "/" + settingsFileName);
		jobToQueue = createJob(settingsFileName, true, true, true);
		//TODO: remove any existing preprocessor and tracker output files
	} else if (currentVideoStage < FlyTracking) {
		QString settingsFileName = "settings_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv";
		getMateBook()->writeTrackerSettings(absoluteDataDirectory().path() + "/" + settingsFileName);
		jobToQueue = createJob(settingsFileName, false, true, true);
		//TODO: remove any existing tracker output files
	} else {
		return;
	}
	currentVideoStage = FlyTracking;
	currentVideoStatus = Queued;
	emit itemChanged(this);
	queueJob(jobToQueue);
}

void FileItem::runPostprocessor()
{
	if (currentVideoStage == FlyTracking && currentVideoStatus == Finished) {
		QString settingsFileName = "settings_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv";
		getMateBook()->writeTrackerSettings(absoluteDataDirectory().path() + "/" + settingsFileName);
		Job* jobToQueue = createJob(settingsFileName, false, false, true);
		//TODO: remove any existing preprocessor and tracker output files
		currentVideoStage = FlyTracking;
		currentVideoStatus = Queued;
		emit itemChanged(this);
		queueJob(jobToQueue);
	}
}

void FileItem::runStatisticalVideoAnalysis()
{
	for (std::vector<ArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		(*iter)->runStatisticalVideoAnalysis();
	}
}

void FileItem::runPulseDetection()
{
	if (currentAudioStage < PulseDetection && currentAudioStatus == Finished) {
		this->pulseDetectionOptions = getMateBook()->getConfigDialog()->getPulseDetectionSettings();
		startPulseDetection();
	}
}

void FileItem::runStatisticalSongAnalysis()
{
	if (currentAudioStage > AudioRecording && currentAudioStatus == Finished) {
		SongResults songResults = getSongResults();
		songResults.reloadData();
		try {
			songResults.calculateStatisticalValues(getMateBook()->getConfigDialog()->getSongAnalysisSettings(), getVideo()->getSampleRate(), getVideo()->getSamples(), getStartTime()*getSampleRate(), getEndTime()*getSampleRate());
		} catch (std::runtime_error& e) {
			QMessageBox::warning(0, tr("Running Statistical Analysis"), e.what());
		}
		songResults.saveData();
		songResults.writeStatisticFile();
		updateSongData(songResults);
		updateStateFromFiles();
	}
}

void FileItem::resetArenaDetection()
{
	if (currentVideoStage >= ArenaDetection) {
		for (std::vector<ArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
			(*iter)->resetArenaDetection();
		}
	}
	absoluteDataDirectory().remove("preprocess_done_success.txt");
	absoluteDataDirectory().remove("preprocess_done_failed.txt");
	absoluteDataDirectory().remove("background.png");
	absoluteDataDirectory().remove("arenas.png");
	updateStateFromFiles();
}

void FileItem::resetFlyTracking()
{
	if (currentVideoStage >= FlyTracking) {
		for (std::vector<ArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
			(*iter)->resetFlyTracking();
		}
	}
	updateStateFromChildren();
}

void FileItem::resetStatisticalVideoAnalysis()
{
	if (currentVideoStage >= FlyTracking) {
		for (std::vector<ArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
			(*iter)->resetStatisticalVideoAnalysis();
		}
	}
	updateStateFromChildren();
}

void FileItem::resetPulseDetection()
{
	if (currentAudioStage > AudioRecording){
		absoluteDataDirectory().remove(QString::fromStdString("songprocess_done_success.txt")); //since for now there is only one audio file per item
		absoluteDataDirectory().remove(QString::fromStdString("songprocess_done_failed.txt")); 
		absoluteDataDirectory().remove(QString::fromStdString("songfile_was_cleaned.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songpulsecenters.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songpulsecycles.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songpulses.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songsines.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songtrains.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songipi.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("raw_songpulsecenters.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("raw_songpulsecycles.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("raw_songpulses.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("raw_songsines.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("raw_songtrains.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("clean_songpulsecenters.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("clean_songpulsecycles.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("clean_songpulses.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("clean_songsines.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("clean_songtrains.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songstatisticsoptions.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("pulsedetectionoptions.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songstatistics.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songbinCycles.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songbinIPI.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songbinTrains.txt"));
		absoluteDataDirectory().remove(QString::fromStdString("songstatistics_done_success.txt"));
		SongResults results = getSongResults();
		updateSongData(results);
		updateStateFromFiles();
	}
}

void FileItem::resetStatisticalSongAnalysis()
{
	if (currentAudioStage > PulseDetection) {
		SongResults results = getSongResults();
		results.reloadRawData();
		updateSongData(results);
		updateStateFromFiles();
	}
}

FileItem::VideoStage FileItem::getCurrentVideoStage() const
{
	return currentVideoStage;
}
FileItem::Status FileItem::getCurrentVideoStatus() const
{
	return currentVideoStatus;
}
FileItem::AudioStage FileItem::getCurrentAudioStage() const
{
	return currentAudioStage;
}
FileItem::Status FileItem::getCurrentAudioStatus() const
{
	return currentAudioStatus;
}

void FileItem::updateStateFromFiles()
{
	removeChildrenKeepData(0, childCount());
	VideoStage newVideoStage = VideoRecording;
	Status newVideoStatus = Finished;
	AudioStage newAudioStage = AudioRecording;
	Status newAudioStatus = Finished;

	if (absoluteDataDirectory().exists("preprocess_done_failed.txt")) {
		newVideoStage = ArenaDetection;
		newVideoStatus = Failed;

		if (!childItems.empty()) {
			std::cerr << "preprocess_done_failed.txt exists, and yet there are arena items: deleting them..." << std::endl;
			for (std::vector<ArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
				//TODO: add some kind of "clearFiles()" call to remove arena data
				delete (*iter);
			}
			childItems.clear();
		}
	} else if (absoluteDataDirectory().exists("preprocess_done_success.txt")) {
		// look for arena files and create an ArenaItem for each one
		QStringList arenaFiles = absoluteDataDirectory().entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

		for (QStringList::const_iterator iter = arenaFiles.constBegin(); iter != arenaFiles.constEnd(); ++iter) {
			try {
				if (QFileInfo(absoluteDataDirectory().filePath(*iter) + "/arena.tsv").exists()) {
					appendChild(new ArenaItem(getMateBook(), getProject(), *iter, this));
				}
			} catch (const RuntimeError& e) {
				std::cerr << "failed to load arena " << iter->toStdString() << ": " << e.what() << std::endl;
			}
		}

		newVideoStage = ArenaDetection;
		newVideoStatus = Finished;
	}

	if(absoluteDataDirectory().exists("songstatistics_done_success.txt")){
		newAudioStage = StatisticalAudioAnalysis;
		newAudioStatus = Finished;
	} else if (absoluteDataDirectory().exists("songstatistics_done_failed.txt")){
		newAudioStage = StatisticalAudioAnalysis;
		newAudioStatus = Failed;
	} else if (absoluteDataDirectory().exists("songfile_was_cleaned.txt")){
		newAudioStage = ModifyingFile;
		newAudioStatus = Finished;
	} else if (absoluteDataDirectory().exists("songprocess_done_success.txt")){
		newAudioStage = PulseDetection;
		newAudioStatus = Finished;
	} else if (absoluteDataDirectory().exists("songprocess_done_failed.txt")){
		newAudioStage = PulseDetection;
		newAudioStatus = Failed;
	}

	// set the state of this item to the most advanced state of any child
	for (std::vector<ArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		VideoStage childStage = (*iter)->getCurrentVideoStage();
		Status childStatus = (*iter)->getCurrentVideoStatus();
		if (childStage > newVideoStage) {
			newVideoStage = childStage;
			newVideoStatus = childStatus;
		} else if (childStage == newVideoStage && childStatus > newVideoStatus) {
			newVideoStatus = childStatus;
		}
	}

	currentVideoStage = newVideoStage;
	currentVideoStatus = newVideoStatus;
	currentAudioStage = newAudioStage;
	currentAudioStatus = newAudioStatus;
	emit itemChanged(this);
}

void FileItem::updateStateFromChildren()
{
	VideoStage newVideoStage = VideoRecording;
	Status newVideoStatus = Finished;

	// set the state of this item to the most advanced state of any child
	for (std::vector<ArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		VideoStage childStage = (*iter)->getCurrentVideoStage();
		Status childStatus = (*iter)->getCurrentVideoStatus();
		if (childStage > newVideoStage) {
			newVideoStage = childStage;
			newVideoStatus = childStatus;
		} else if (childStage == newVideoStage && childStatus > newVideoStatus) {
			newVideoStatus = childStatus;
		}
	}

	if (newVideoStage != currentVideoStage || newVideoStatus != currentVideoStatus) {
		currentVideoStage = newVideoStage;
		currentVideoStatus = newVideoStatus;
	}
	emit itemChanged(this);
}

void FileItem::jobStarted(Job* job)
{
	currentVideoStatus = Started;
	currentAudioStatus = Started;
	emit itemChanged(this);
}

void FileItem::jobFinished(Job* job)
{
	currentVideoStatus = Finished;
	currentAudioStatus = Finished;
	SongResults results = getSongResults();
	results.reloadData();
	updateSongData(results);
	updateStateFromFiles();
	emit itemChanged(this);
}

void FileItem::jobError(Job* job)
{
	currentVideoStatus = Failed;
	currentAudioStatus = Failed;
	emit itemChanged(this);
}

void FileItem::childItemChanged(ArenaItem* child)
{
	emit itemChanged(child);
	updateStateFromChildren();
}

// saves the options that were used for the analysis, creates a parameter string
// this string is used for the matlab process and to be able to identify if two
// files ran with the same parametrs (saved in video.tsv)
void FileItem::startPulseDetection(){
	QString newPath(videoFilePath.c_str()); 

	if(!newPath.isEmpty()){
		currentAudioStage = PulseDetection;
		currentAudioStatus = Queued;
 
		try{
			getSongResults().writePulseDetectionOptions(pulseDetectionOptions);
		}catch(std::runtime_error &e){
			QMessageBox::critical(0, QObject::tr("Writing File:"), e.what());
		}

		QStringList param;
		param
			<< QString::number(pulseDetectionOptions["TapersTimeBandwith"]) << QString::number(pulseDetectionOptions["IndependentTapersCount"]) 
			<< QString::number(pulseDetectionOptions["WindowStepSize"]) << QString::number(pulseDetectionOptions["WindowLength"]) << QString::number(pulseDetectionOptions["FTestCrit"]) 
			<< QString::number(pulseDetectionOptions["LowestSineFS"]) << QString::number(pulseDetectionOptions["HighestSineFS"]) << QString::number(pulseDetectionOptions["SineRangePerc"]) 
			<< QString::number(pulseDetectionOptions["MinSineSize"]) << QString::number(pulseDetectionOptions["NoiseCutofF"]) << QString::number(pulseDetectionOptions["TrainExpandRange"]) 
			<< QString::number(pulseDetectionOptions["CombineStepSize"]) << QString::number(pulseDetectionOptions["CutoffFactor"]) << QString::number(pulseDetectionOptions["MinPulseHeight"]) 
			<< QString::number(pulseDetectionOptions["MaxPulseScaleFreq"]) << QString::number(pulseDetectionOptions["MinDogSperation"]) << QString::number(pulseDetectionOptions["MinMorletSeperation"]) 
			<< QString::number(pulseDetectionOptions["PeakExpansionFact"]) << QString::number(pulseDetectionOptions["PeakToPeakVoltage"]) << QString::number(pulseDetectionOptions["MaxPulseDistance"]) 
			<< QString::number(pulseDetectionOptions["MinPulseDistance"]) << QString::number(pulseDetectionOptions["PulseTimeWindow"]) << QString::number(pulseDetectionOptions["Exclude0Cycles"]);
		songOptionId = noiseFileName + " " + QString(param.join(" ")).toStdString(); // id to see if songs ran with same options
		
		QStringList arguments;
		
		#if defined (__APPLE__)
			QString flysongExecutable = QFileInfo(global::executableDir + "/run_flysong.sh").canonicalFilePath();
			#if !defined(_DEBUG)
				arguments << "/Applications/MATLAB/MATLAB_Compiler_Runtime/v715";
			#endif
		#else
			QString flysongExecutable = QFileInfo(global::executableDir + "/flysong.exe").canonicalFilePath();
		#endif
		QDir workingDirectory(absoluteDataDirectory());
		#if defined(_DEBUG)
			flysongExecutable = getMateBook()->getConfigDialog()->getMatlabExecutable();
			#if defined(__APPLE__)
				arguments << "-r" << "Process_Song_Save('" + absoluteDataDirectory().absolutePath() + "', '" + newPath + "', '" + QString::fromStdString(noiseFileName) + "', " + param.join(", ") + ");" /*<< "-logfile" << absoluteDataDirectory().absolutePath() + QString("/matlab.log")*/ << "-desktop";
				workingDirectory = QDir(global::executableDir + "../../../../../../../flysong/source");
			#else
				arguments << "-r" << "Process_Song_Save('" + absoluteDataDirectory().absolutePath() + "', '" + newPath + "', '" + QString::fromStdString(noiseFileName) + "', " + param.join(", ") + ");" << "-wait";
				workingDirectory = QDir("../../flysong/source");
			#endif
		#else
			arguments << absoluteDataDirectory().absolutePath() << newPath << QString::fromStdString(noiseFileName) << param;
		#endif
		
		Job* jobToQueue = new ExternalJob(flysongExecutable, arguments, workingDirectory);

		connect(jobToQueue, SIGNAL(jobStarted(Job*)), this, SLOT(jobStarted(Job*)));
		connect(jobToQueue, SIGNAL(jobFinished(Job*)), this, SLOT(jobFinished(Job*)));
		connect(jobToQueue, SIGNAL(jobError(Job*)), this, SLOT(jobError(Job*)));
		Singleton<JobQueue>::instance().queueJob(jobToQueue);
	}
}

const std::vector<ArenaItem*>& FileItem::getChildItems() const
{
	return childItems;
}

void FileItem::createArena(QRect boundingBox)
{
	boundingBox = boundingBox.intersected(QRect(0, 0, width, height));	// clip to video size
	QString arenaId = "CustomArena_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz");
	try {
		appendChild(new ArenaItem(getMateBook(), getProject(), arenaId, boundingBox, getMateBook()->getConfigDialog()->getArenaDiameter(), this));
		emit itemChanged(this);
	} catch (RuntimeError& e) {
		QMessageBox::warning(getMateBook(), tr("Creating arena"), e.what());
	}
}

// reload the data from all files into songResults
void FileItem::updateSongData(const SongResults& results)
{
	pulses = results.getPulses().size();
	sines = results.getSines().size();
	trains = results.getTrains().size();
	pulseCenters = results.getPulseCenters().size();

	videoInfo.exportTo(absoluteDataDirectory().filePath("video.tsv").toStdString());
}

void FileItem::registerSettings()
{
	videoInfo.add("path", videoFilePath);
	videoInfo.add("width", width);
	videoInfo.add("height", height);
	videoInfo.add("frameCount", numFrames);
	videoInfo.add("fps", fps);
	videoInfo.add("startTime", startTime);
	videoInfo.add("endTime", endTime);
	videoInfo.add("experimenter", experimenter);
//TODO: fileDate	videoInfo.add("", );
/* from here:
	fileDate = QDateTime::fromString(QString::fromStdString(videoInfo.get("fileDate").as<std::string>()));
*/
/* from FileItem::serialize():
	videoInfo.set("fileDate", fileDate.toString().toStdString());
*/
	videoInfo.add("noiseFileName", noiseFileName);
	videoInfo.add("samples", samples);
	videoInfo.add("sampleRate", sampleRate);
	videoInfo.add("pulses", pulses);
	videoInfo.add("sines", sines);
	videoInfo.add("trains", trains);
	videoInfo.add("pulseCenters", pulseCenters);
	videoInfo.add("songOptionId", songOptionId);
	videoInfo.add("firstSex", firstSex);
	videoInfo.add("firstGenotype", firstGenotype);
	videoInfo.add("secondSex", secondSex);
	videoInfo.add("secondGenotype", secondGenotype);
	videoInfo.add("comment", comment);
}

QStringList FileItem::getArguments(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const
{
	QString trackerArgumentPrefix = getMateBook()->getConfigDialog()->getArgumentPrefix();
	QString currentVideoFileName = getFileName();
	QString currentOutDir = absoluteDataDirectory().absolutePath();
	#if defined(WIN32)
		// for paths we have to replace each \ with a / on Windows
		currentVideoFileName.replace('\\', "/");
		currentOutDir.replace('\\', "/");
	#endif
	QStringList arguments;
	arguments
		<< trackerArgumentPrefix + "guiversion" << QString::number(Version::current)
		<< trackerArgumentPrefix + "in" << currentVideoFileName
		<< trackerArgumentPrefix + "out" << currentOutDir
		<< trackerArgumentPrefix + "settings" << settingsFileName
		<< trackerArgumentPrefix + "preprocess" << QString::fromStdString(stringify(preprocess))
		<< trackerArgumentPrefix + "track" << QString::fromStdString(stringify(track))
		<< trackerArgumentPrefix + "postprocess" << QString::fromStdString(stringify(postprocess))
		<< trackerArgumentPrefix + "begin" << QString::number(getStartTime())
		<< trackerArgumentPrefix + "end" << QString::number(getEndTime())
	;
	return arguments;
}

Job* FileItem::createJob(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const
{
	QStringList arguments = getArguments(settingsFileName, preprocess, track, postprocess);
	if (getMateBook()->getConfigDialog()->isClusterProcessingEnabled() && canUseCluster()) {
		return new ClusterJob(
			getMateBook()->getConfigDialog()->getSshClient(),
			getMateBook()->getConfigDialog()->getSshTransferHost(),
			getMateBook()->getConfigDialog()->getSshUsername(),
			getMateBook()->getConfigDialog()->getSshPrivateKey(),
			getMateBook()->getConfigDialog()->getSshEnvironment(),
			getMateBook()->getConfigDialog()->getPollingInterval(),
			QString("/projects/DIK.screen/tracker/") + QString::number(Version::current) + "/qsub_track.sh",
			arguments,
			absoluteDataDirectory()
		);
	} else {
		return new ExternalJob(getMateBook()->getConfigDialog()->getTrackerExecutable(), arguments, absoluteDataDirectory());
	}
}

void FileItem::queueJob(Job* jobToQueue) const
{
	connect(jobToQueue, SIGNAL(jobStarted(Job*)), this, SLOT(jobStarted(Job*)));
	connect(jobToQueue, SIGNAL(jobFinished(Job*)), this, SLOT(jobFinished(Job*)));
	connect(jobToQueue, SIGNAL(jobError(Job*)), this, SLOT(jobError(Job*)));
	Singleton<JobQueue>::instance().queueJob(jobToQueue);
}

bool FileItem::canUseCluster() const
{
	#if defined(WIN32)
		return (getFileName().startsWith("\\\\") || getFileName().startsWith("//")) &&
			(absoluteDataDirectory().absolutePath().startsWith("\\\\") || absoluteDataDirectory().absolutePath().startsWith("//"));
	#else
		return getFileName().startsWith("/Volumes/") && absoluteDataDirectory().absolutePath().startsWith("/Volumes/");
	#endif
}
