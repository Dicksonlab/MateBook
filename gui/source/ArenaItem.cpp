#include "ArenaItem.hpp"
#include <QStringList>
#include <QFileInfo>
#include <QTextStream>
#include <QImageReader>
#include <iostream>
#include <numeric>
#include <cassert>
#include "RuntimeError.hpp"
#include "../../common/source/Singleton.hpp"
#include "ExternalJob.hpp"
#include "ClusterJob.hpp"
#include "JobQueue.hpp"
#include "FileItem.hpp"
#include "MateBook.hpp"
#include "Project.hpp"
#include "FileUtilities2.hpp"
#include "Version.hpp"
#include "ConfigDialog.hpp"
#include "../../common/source/serialization.hpp"
#include "../../common/source/mathematics.hpp"

// new item; its directory will be created
ArenaItem::ArenaItem(MateBook* mateBook, Project* project, const QString& arenaDirName, QRect boundingBox, float diameter, FileItem* parent) : Item(mateBook, project),
	parentItem(parent),
	left(boundingBox.left()),
	top(boundingBox.top()),
	width(boundingBox.width()),
	height(boundingBox.height()),
	diameter(diameter),
	approved(true),
	id(arenaDirName.toStdString()),
	currentVideoStage(ArenaDetection),
	currentVideoStatus(Finished),
	currentAudioStage(AudioRecording),
	currentAudioStatus(Finished),
	startTime(),
	endTime(),
	courtship(),
	quality()
{
	registerSettings();

	startTime = parentItem->getStartTime();
	endTime = parentItem->getEndTime();

	if (!width || !height) {
		throw RuntimeError(QObject::tr("Arena description is incomplete: width and height must be greater than 0.").arg(QString::fromStdString(id)));
	}

	if (!parent->absoluteDataDirectory().mkdir(QString::fromStdString(id))) {
		throw RuntimeError(QObject::tr("Could not create data directory for arena %1. Do you have write permissions?").arg(QString::fromStdString(id)));
	}

	updateStateFromFiles();

	serialize();	// For Salil. ;)
}

// arena.tsv existing on disk
ArenaItem::ArenaItem(MateBook* mateBook, Project* project, const QString& arenaDirName, FileItem* parent) : Item(mateBook, project),
	parentItem(parent),
	left(),
	top(),
	width(),
	height(),
	diameter(),
	approved(true),
	id(arenaDirName.toStdString()),
	currentVideoStage(ArenaDetection),
	currentVideoStatus(Finished),
	currentAudioStage(AudioRecording),
	currentAudioStatus(Finished),
	startTime(),
	endTime(),
	courtship(),
	quality()
{
	registerSettings();

	startTime = parentItem->getStartTime();
	endTime = parentItem->getEndTime();

	try {
		arenaInfo.importFrom(absoluteDataDirectory().filePath("arena.tsv").toStdString());
	} catch (const std::bad_cast& e) {
		throw RuntimeError(QObject::tr("Could not parse %1.").arg(absoluteDataDirectory().filePath("arena.tsv")));
	}
	//TODO: what exception is thrown if the file cannot be read? where is it caught?

	if (!width || !height) {
		throw RuntimeError(QObject::tr("Arena description in %1 is incomplete: width or height missing.").arg(QString::fromStdString(id)));
	}

	updateStateFromFiles();
}

ArenaItem::~ArenaItem()
{
}

//TODO: see if we can remove this method from the Item interface - this is bad OO design)
void ArenaItem::appendChild(ArenaItem *item)
{
	assert(false);	// an ArenaItem doesn't have any children
}

//TODO: see if we can remove this method from the Item interface - this is bad OO design)
ArenaItem* ArenaItem::child(int row) const
{
	assert(false);	// an ArenaItem doesn't have any children
	return NULL;
}

int ArenaItem::childCount() const
{
	return 0;
}

Item* ArenaItem::parent()
{
	return parentItem;
}

void ArenaItem::removeChildren(int position, int count)
{
	assert(count == 0);	// an ArenaItem doesn't have any children
}

int ArenaItem::childIndex() const
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

void ArenaItem::removeData()
{
	try {
		FileUtilities2::removeDirectory(absoluteDataDirectory());
	} catch (RuntimeError& e) {
		std::cerr << "ArenaItem::removeData: " << e.what() << std::endl;
	}
}

boost::shared_ptr<Video> ArenaItem::getVideo() const
{
	return parentItem->getVideo();
}

boost::shared_ptr<TrackingResults> ArenaItem::getTrackingResults() const
{
	std::string filePath = absoluteDataDirectory().filePath("track").toStdString();
	std::string contourFilePath = absoluteDataDirectory().filePath("contour.bin").toStdString();
	std::string smoothHistogramFilePath = absoluteDataDirectory().filePath("smoothHistogram.bin256f").toStdString();
	return boost::shared_ptr<TrackingResults>(new TrackingResults(filePath, contourFilePath, smoothHistogramFilePath));
}

QImage ArenaItem::getEthogram(unsigned int flyNumber) const
{
	QString fileName = QString::fromStdString(stringify(flyNumber) + "_ethoTableCell.png");
	return QImage(absoluteDataDirectory().filePath(fileName));
}

QSize ArenaItem::getEthogramSize(unsigned int flyNumber) const
{
	QString fileName = QString::fromStdString(stringify(flyNumber) + "_ethoTableCell.png");
	QString filePath = absoluteDataDirectory().filePath(fileName);
	return QImageReader(filePath).size();
}

QString ArenaItem::getFileName() const
{
	return parentItem->getFileName();
}

void ArenaItem::setFileName(const QString& name)
{
	parentItem->setFileName(name);
}

QString ArenaItem::getAnnotationFileName() const
{
	return absoluteDataDirectory().filePath("annotation.tsv");
}

std::string ArenaItem::getId() const
{
	return id;
}

unsigned int ArenaItem::getLeft() const
{
	return left;
}

unsigned int ArenaItem::getTop() const
{
	return top;
}

unsigned int ArenaItem::getWidth() const
{
	return width;
}

unsigned int ArenaItem::getHeight() const
{
	return height;
}

unsigned int ArenaItem::getNumFrames() const
{
	return parentItem->getNumFrames();
}

double ArenaItem::getFps() const
{
	return parentItem->getFps();
}

QString ArenaItem::getNoiseFileName() const
{
	return parentItem->getNoiseFileName();
}

void ArenaItem::setNoiseFileName(const QString& name)
{
	parentItem->setNoiseFileName(name);
}

unsigned int ArenaItem::getSamples() const
{
	return parentItem->getSamples();
}

unsigned int ArenaItem::getSampleRate() const
{
	return parentItem->getSampleRate();
}

unsigned int ArenaItem::getPulses() const
{
	return parentItem->getPulses();
}

unsigned int ArenaItem::getSines() const
{
	return parentItem->getSines();
}

unsigned int ArenaItem::getTrains() const
{
	return parentItem->getTrains();
}

unsigned int ArenaItem::getPulseCenters() const
{
	return parentItem->getPulseCenters();
}

QString ArenaItem::getSongOptionId() const
{
	return parentItem->getSongOptionId();
}

std::map<QString, float> ArenaItem::getPulseDetectionOptions()
{
	return parentItem->getPulseDetectionOptions();
}

std::map<QString, float> ArenaItem::getSongStatisticsOptions()
{
	return parentItem->getSongStatisticsOptions();
}

unsigned int ArenaItem::getDuration() const
{
	return parentItem->getDuration();
}

QDateTime ArenaItem::getDate() const
{
	return parentItem->getDate();
}

void ArenaItem::setDate(const QDateTime& date)
{
	parentItem->setDate(date);
}

std::string ArenaItem::getFirstSex() const
{
	return firstSex;
}

void ArenaItem::setFirstSex(const std::string& firstSex)
{
	this->firstSex = firstSex;
}

std::string ArenaItem::getFirstGenotype() const
{
	return firstGenotype;
}

void ArenaItem::setFirstGenotype(const std::string& firstGenotype)
{
	this->firstGenotype = firstGenotype;
}

std::string ArenaItem::getSecondSex() const
{
	return secondSex;
}

void ArenaItem::setSecondSex(const std::string& secondSex)
{
	this->secondSex = secondSex;
}

std::string ArenaItem::getSecondGenotype() const
{
	return secondGenotype;
}

void ArenaItem::setSecondGenotype(const std::string& secondGenotype)
{
	this->secondGenotype = secondGenotype;
}

unsigned int ArenaItem::getStartTime() const
{
	return startTime;
}

void ArenaItem::setStartTime(const unsigned int time)
{
	startTime = std::min(time, endTime);
}

unsigned int ArenaItem::getEndTime() const
{
	return endTime;
}

void ArenaItem::setEndTime(const unsigned int time)
{
	endTime = std::min(time, getDuration());
	endTime = std::max(startTime, endTime);
}

std::string ArenaItem::getExperimenter() const
{
	return parentItem->getExperimenter();
}

void ArenaItem::setExperimenter(const std::string& experimenter)
{
	parentItem->setExperimenter(experimenter);
}

unsigned int ArenaItem::getApproved() const
{
	return approved;
}

void ArenaItem::setApproved(bool approved)
{
	this->approved = approved;
	emit itemChanged(this);
}

std::string ArenaItem::getComment() const
{
	return comment;
}

void ArenaItem::setComment(const std::string& comment)
{
	this->comment = comment;
}

bool ArenaItem::hasCourtship() const
{
	return (currentVideoStage == FlyTracking && currentVideoStatus == Finished) || currentVideoStage > FlyTracking;
}

double ArenaItem::getCourtship() const
{
	return courtship;
}

bool ArenaItem::hasQuality() const
{
	return (currentVideoStage == FlyTracking && currentVideoStatus == Finished) || currentVideoStage > FlyTracking;
}

double ArenaItem::getQuality() const
{
	return quality;
}

QPixmap ArenaItem::getFirstEthogram() const
{
	return firstEthogram;
}

QPixmap ArenaItem::getSecondEthogram() const
{
	return secondEthogram;
}

void ArenaItem::serialize() const
{
	Item::serialize();
	arenaInfo.exportTo(absoluteDataDirectory().filePath("arena.tsv").toStdString());

	// call serialize() on members here if necessary
}

QDir ArenaItem::relativeDataDirectory() const
{
	return QDir(parentItem->relativeDataDirectory().filePath(QString::fromStdString(id)));
}

QDir ArenaItem::absoluteDataDirectory() const
{
	return QDir(parentItem->absoluteDataDirectory().filePath(QString::fromStdString(id)));
}

void ArenaItem::runArenaDetection()
{
	//TODO: bad OO design here - Processing should be switched to a more dynamic model, where Items enumerate the possible stages
}

void ArenaItem::runFlyTracking()
{
	QString trackerArgumentPrefix = getMateBook()->getConfigDialog()->getArgumentPrefix();
	if (currentVideoStage < FlyTracking) {
		QString settingsFileName = "settings_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv";
		getMateBook()->writeTrackerSettings(absoluteDataDirectory().filePath(settingsFileName));
		Job* jobToQueue = createJob(settingsFileName, false, true, true);	//TODO: pass the full path for the settings file
		//TODO: remove any existing tracker output files
		currentVideoStage = FlyTracking;
		currentVideoStatus = Queued;
		emit itemChanged(this);
		queueJob(jobToQueue);
	}
}

void ArenaItem::runPostprocessor()
{
	if (currentVideoStage == FlyTracking && currentVideoStatus == Finished) {
		QString settingsFileName = "settings_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv";
		getMateBook()->writeTrackerSettings(absoluteDataDirectory().filePath(settingsFileName));
		Job* jobToQueue = createJob(settingsFileName, false, false, true);	//TODO: pass the full path for the settings file
		//TODO: remove any existing tracker output files
		currentVideoStage = FlyTracking;
		currentVideoStatus = Queued;
		emit itemChanged(this);
		queueJob(jobToQueue);
	}
}

void ArenaItem::runStatisticalVideoAnalysis()
{
//TODO: figure out how to deal with different attribute types (maybe make this as a virtual member of AbstractAttribute)
/*	if (getCurrentVideoStage() == Item::FlyTracking && getCurrentVideoStatus() == Item::Finished) {
		try {
			const size_t endFrame = static_cast<size_t>(getFps() * getEndTime());
			boost::shared_ptr<TrackingResults> trackingResults = getTrackingResults();
			std::string meanFileName = absoluteDataDirectory().canonicalPath().toStdString() + "/" + stringify(getId()) + "_mean.tsv";
			std::ofstream meanFile(meanFileName.c_str());
			bool firstValueWritten = false;
			{	// per-frame means
				std::vector<std::string> frameAttributeNames = trackingResults->getFrameAttributeNames();
				for (std::vector<std::string>::const_iterator iter = frameAttributeNames.begin(); iter != frameAttributeNames.end(); ++iter) {
					const std::vector<float>& attribute = trackingResults->getFrameData(*iter);
					float meanValue = mean(attribute.begin(), (std::distance(attribute.begin(), attribute.end()) < endFrame ? attribute.end() : attribute.begin() + endFrame));
					if (firstValueWritten) {
						meanFile << '\t';
					} else {
						firstValueWritten = true;
					}
					meanFile << meanValue;
				}
			}
			{	// per-fly means
				std::vector<std::string> flyAttributeNames = trackingResults->getFlyAttributeNames();
				for (size_t flyNumber = 0; flyNumber != trackingResults->getFlyCount(); ++flyNumber) {
					for (std::vector<std::string>::const_iterator iter = flyAttributeNames.begin(); iter != flyAttributeNames.end(); ++iter) {
						const std::vector<float>& attribute = trackingResults->getFlyData(flyNumber, *iter);
						float meanValue = mean(attribute.begin(), (std::distance(attribute.begin(), attribute.end()) < endFrame ? attribute.end() : attribute.begin() + endFrame));
						if (firstValueWritten) {
							meanFile << '\t';
						} else {
							firstValueWritten = true;
						}
						meanFile << meanValue;
					}
				}
			}
			meanFile << '\n';
			QFile(absoluteDataDirectory().canonicalPath() + "/" + QString::fromStdString(stringify(id) + "_stats_done_success.txt")).open(QIODevice::WriteOnly);
			currentVideoStage = Item::StatisticalVideoAnalysis;
		} catch (RuntimeError& e) {
			//TODO: give the user an error message?
			QFile(absoluteDataDirectory().canonicalPath() + "/" + QString::fromStdString(stringify(id) + "_stats_done_failed.txt")).open(QIODevice::WriteOnly);
			currentVideoStage = Item::StatisticalVideoAnalysis;
			currentVideoStatus = Item::Failed;
		}
		emit itemChanged(this);
	}
*/
}

void ArenaItem::runPulseDetection()
{
	parentItem->runPulseDetection();
}

void ArenaItem::runStatisticalSongAnalysis()
{
	parentItem->runStatisticalSongAnalysis();
}

void ArenaItem::resetArenaDetection()
{
	resetFlyTracking();
	if (currentVideoStage >= ArenaDetection) {
		absoluteDataDirectory().remove("arena.tsv");
		absoluteDataDirectory().remove("mask.png");
		absoluteDataDirectory().remove("smoothBackground.png");
	}
	currentVideoStage = VideoRecording;
	currentVideoStatus = Finished;
}

void ArenaItem::resetFlyTracking()
{
	resetStatisticalVideoAnalysis();
	if (currentVideoStage >= FlyTracking) {
		absoluteDataDirectory().remove("track_done_success.txt");
		absoluteDataDirectory().remove("track_done_failed.txt");
		absoluteDataDirectory().remove("track.tsv");
		QString binariesDirPath = absoluteDataDirectory().filePath("track");
		try {
			FileUtilities2::removeDirectory(QDir(binariesDirPath));
		} catch (RuntimeError& e) {
				std::cerr << "ArenaItem::resetFlyTracking: " << e.what() << std::endl;
		}
		absoluteDataDirectory().remove("contour.bin");
		absoluteDataDirectory().remove("smoothHistogram.bin256f");
		absoluteDataDirectory().remove("mean.tsv");
		absoluteDataDirectory().remove("missegmented.tsv");
		absoluteDataDirectory().remove("positionCorrelation.tsv");
		absoluteDataDirectory().remove("segmentationStatistics.tsv");
	}
	updateStateFromFiles();
}

void ArenaItem::resetStatisticalVideoAnalysis()
{
	if (currentVideoStage >= StatisticalVideoAnalysis) {
		//TODO: update this list once we have a proper statistical analysis
//		parentItem->absoluteDataDirectory().remove(QString::fromStdString(stringify(id) + "_stats_done_success.txt"));
//		parentItem->absoluteDataDirectory().remove(QString::fromStdString(stringify(id) + "_stats_done_failed.txt"));
//		parentItem->absoluteDataDirectory().remove(QString::fromStdString(stringify(id) + "_mean.tsv"));
	}
	updateStateFromFiles();
}

void ArenaItem::resetPulseDetection()
{
	parentItem->resetPulseDetection();
}

void ArenaItem::resetStatisticalSongAnalysis()
{
	parentItem->resetStatisticalSongAnalysis();
}

ArenaItem::VideoStage ArenaItem::getCurrentVideoStage() const
{
	return currentVideoStage;
}

ArenaItem::Status ArenaItem::getCurrentVideoStatus() const
{
	return currentVideoStatus;
}

ArenaItem::AudioStage ArenaItem::getCurrentAudioStage() const
{
	return currentAudioStage;
}

ArenaItem::Status ArenaItem::getCurrentAudioStatus() const
{
	return currentAudioStatus;
}

void ArenaItem::updateStateFromFiles()
{
	VideoStage newStage = ArenaDetection;
	Status newStatus = Finished;

	//TODO: handle current status Started and Queued
	//TODO: handle inconsistent files (like: make sure we have a trackZ, if there's a postprocess_done_success)

	if (absoluteDataDirectory().exists("track_done_failed.txt")) {
		newStage = FlyTracking;
		newStatus = Failed;
	} else if (absoluteDataDirectory().exists("track_done_success.txt")) {
		newStage = FlyTracking;
		newStatus = Finished;

		// read courtship index and tracking quality from behavior file
		QString behaviorFilePath = absoluteDataDirectory().filePath("behavior.tsv");
		QFile behaviorFile(behaviorFilePath);
		if (!behaviorFile.open(QFile::ReadOnly)) {
			std::cerr << (QString("Could not be open ") + behaviorFilePath + QString(" for reading.")).toStdString();
		} else {
			QTextStream textStream(&behaviorFile);
			QString headersLine = textStream.readLine();
			QString dataLine = textStream.readLine();
			QStringList headers = headersLine.split('\t');
			QStringList data = dataLine.split('\t');

			if (headers.size() != data.size()) {
				std::cerr << (QString("Headers and data do not match in file ") + behaviorFilePath + QString(".")).toStdString();
			} else {
				int courtshipIndex = headers.indexOf("courtship");
				if (courtshipIndex != -1) {
					courtship = data[courtshipIndex].toDouble();
				}

				int qualityIndex = headers.indexOf("quality");
				if (qualityIndex != -1) {
					quality = data[qualityIndex].toDouble();
				}
			}
		}

		// load ethograms
		firstEthogram.load(absoluteDataDirectory().filePath("0_ethoTableCell.png"));
		secondEthogram.load(absoluteDataDirectory().filePath("1_ethoTableCell.png"));

		if (absoluteDataDirectory().exists("stats_done_failed.txt")) {
			newStage = StatisticalVideoAnalysis;
			newStatus = Failed;
		} else if (absoluteDataDirectory().exists("stats_done_success.txt")) {
			newStage = StatisticalVideoAnalysis;
			newStatus = Finished;
		}
	}

	currentVideoStage = newStage;
	currentVideoStatus = newStatus;
	emit itemChanged(this);
}

void ArenaItem::cat(const QString& sourceFileName, QTextStream& destination, bool prefixWithArenaId, bool hasHeaders, bool writeHeaders) const
{
	QString sourceFilePath = absoluteDataDirectory().filePath(sourceFileName);
	QFile sourceFile(sourceFilePath);
	if (!sourceFile.open(QFile::ReadOnly)) {
		throw RuntimeError(QObject::tr("%1 could not be opened for reading in cat().").arg(sourceFileName));
	}

	QTextStream textStream(&sourceFile);
	if (hasHeaders) {
		if (writeHeaders) {
			if (prefixWithArenaId) {
				destination << "File" << '\t' << "Arena" << '\t';
			}
			destination << textStream.readLine() << '\n';
		} else {
			textStream.readLine();
		}
	}
	if (prefixWithArenaId) {
		destination << getFileName() << '\t' << QString::fromStdString(getId()) << '\t';
	}
	destination << textStream.readAll();
}

void ArenaItem::jobStarted(Job* job)
{
	currentVideoStatus = Started;
	emit itemChanged(this);
}

void ArenaItem::jobFinished(Job* job)
{
	currentVideoStatus = Finished;
	updateStateFromFiles();
	emit itemChanged(this);
}

void ArenaItem::jobError(Job* job)
{
	currentVideoStatus = Failed;
	emit itemChanged(this);
}

QString ArenaItem::intSecondsToTimeString(const unsigned int t) const
{
	unsigned int tempTime = t;
	unsigned int hours = tempTime / 3600;
	tempTime -= hours * 3600;
	unsigned int minutes = tempTime / 60;
	tempTime -= minutes * 60;
	unsigned int seconds = tempTime;

	return QString::number(hours).rightJustified(2, '0') + ":" + QString::number(minutes).rightJustified(2, '0') + ":" + 
		   QString::number(seconds).rightJustified(2, '0');
}

void ArenaItem::registerSettings()
{
	arenaInfo.add("left", left);
	arenaInfo.add("top", top);
	arenaInfo.add("width", width);
	arenaInfo.add("height", height);
	arenaInfo.add("approved", approved);
	arenaInfo.add("diameter", diameter);
	arenaInfo.add("startTime", startTime);
	arenaInfo.add("endTime", endTime);
}

QStringList ArenaItem::getArguments(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const
{
	QString trackerArgumentPrefix = getMateBook()->getConfigDialog()->getArgumentPrefix();
	QString currentVideoFileName = getFileName();
	QString currentOutDir = parentItem->absoluteDataDirectory().absolutePath();
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
		<< trackerArgumentPrefix + "arena" << QString::fromStdString(getId())	// should be last as it's optional and qsub_track.sh accepts arguments based on position, not name
	;
	return arguments;
}

Job* ArenaItem::createJob(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const
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
			QString("/tier2/dickson/dicksonlab/MateBook/MateBook/usr/bin/tracker/") + QString::number(Version::current) + "/qsub_track.sh",
			arguments,
			parentItem->absoluteDataDirectory()
		);
	} else {
		return new ExternalJob(getMateBook()->getConfigDialog()->getTrackerExecutable(), arguments, parentItem->absoluteDataDirectory());
	}
}

void ArenaItem::queueJob(Job* jobToQueue) const
{
	connect(jobToQueue, SIGNAL(jobStarted(Job*)), this, SLOT(jobStarted(Job*)));
	connect(jobToQueue, SIGNAL(jobFinished(Job*)), this, SLOT(jobFinished(Job*)));
	connect(jobToQueue, SIGNAL(jobError(Job*)), this, SLOT(jobError(Job*)));
	Singleton<JobQueue>::instance().queueJob(jobToQueue);
}

bool ArenaItem::canUseCluster() const
{
	return false;	//TODO: our scripts cannot pass the arena parameter through at this time
/*	#if defined(WIN32)
		return (getFileName().startsWith("\\\\") || getFileName().startsWith("//")) &&
			(absoluteDataDirectory().absolutePath().startsWith("\\\\") || absoluteDataDirectory().absolutePath().startsWith("//"));
	#else
		return getFileName().startsWith("/Volumes/") && absoluteDataDirectory().absolutePath().startsWith("/Volumes/");
	#endif
*/
}
