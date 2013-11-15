#ifndef ArenaItem_hpp
#define ArenaItem_hpp

#include <QList>
#include <QVariant>
#include <QPixmap>
#include <QTime>
#include <QDir>

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "Item.hpp"
#include "Video.hpp"
#include "TrackingResults.hpp"
#include "../../common/source/Settings.hpp"

class Job;
class FileItem;
class MateBook;
class Project;
class QTextStream;

class ArenaItem : public Item {
	Q_OBJECT

public:
	ArenaItem(MateBook* mateBook, Project* project, const QString& arenaDirName, QRect boundingBox, float diameter, FileItem* parent);	// new item; its directory will be created
	ArenaItem(MateBook* mateBook, Project* project, const QString& arenaDirName, FileItem* parent);	// arena.tsv existing on disk

	~ArenaItem();

	Item* parent();
	void appendChild(ArenaItem* child);
	ArenaItem* child(int row) const;
	int childCount() const;
	void removeChildren(int position, int count);
	int childIndex() const;	// the parent's child index for this item

	void removeData();

	boost::shared_ptr<Video> getVideo() const;
	boost::shared_ptr<TrackingResults> getTrackingResults() const;
	QImage getEthogram(unsigned int flyNumber) const;
	QSize getEthogramSize(unsigned int flyNumber) const;

	// arena information
	QString getFileName() const;
	void setFileName(const QString& name);
	QString getAnnotationFileName() const;
	std::string getId() const;
	unsigned int getLeft() const;
	unsigned int getTop() const;
	unsigned int getWidth() const;
	unsigned int getHeight() const;
	unsigned int getNumFrames() const;
	double getFps() const;

	// song information
	QString getNoiseFileName() const;
	void setNoiseFileName(const QString& name);
	unsigned int getSamples() const;
	unsigned int getSampleRate() const;
	unsigned int getPulses() const;
	unsigned int getSines() const;
	unsigned int getTrains() const;
	unsigned int getPulseCenters() const;
	QString getSongOptionId() const;
	std::map<QString, float> getPulseDetectionOptions();
	std::map<QString, float> getSongStatisticsOptions();

	unsigned int getDuration() const;
	QDateTime getDate() const;
	void setDate(const QDateTime& date);

	std::string getFirstSex() const;
	void setFirstSex(const std::string& firstSex);
	std::string getFirstGenotype() const;
	void setFirstGenotype(const std::string& firstGenotype);
	std::string getSecondSex() const;
	void setSecondSex(const std::string& secondSex);
	std::string getSecondGenotype() const;
	void setSecondGenotype(const std::string& secondGenotype);

	unsigned int getStartTime() const;
	void setStartTime(const unsigned int time);
	unsigned int getEndTime() const;
	void setEndTime(const unsigned int time);

	std::string getExperimenter() const;
	void setExperimenter(const std::string& experimenter);

	unsigned int getApproved() const;
	void setApproved(bool approved);

	std::string getComment() const;
	void setComment(const std::string& comment);

	// tracking results
	bool hasCourtship() const;
	double getCourtship() const;
	bool hasQuality() const;
	double getQuality() const;
	QPixmap getFirstEthogram() const;
	QPixmap getSecondEthogram() const;

	void serialize() const;

	QDir relativeDataDirectory() const;
	QDir absoluteDataDirectory() const;

	void runArenaDetection();
	void runFlyTracking();
	void runPostprocessor();
	void runStatisticalVideoAnalysis();
	void runPulseDetection();
	void runStatisticalSongAnalysis();

	void resetArenaDetection();
	void resetFlyTracking();
	void resetStatisticalVideoAnalysis();
	void resetPulseDetection();
	void resetStatisticalSongAnalysis();

	ArenaItem::VideoStage getCurrentVideoStage() const;
	ArenaItem::Status getCurrentVideoStatus() const;
	ArenaItem::AudioStage getCurrentAudioStage() const;
	ArenaItem::Status getCurrentAudioStatus() const;

	void updateStateFromFiles();

	void cat(const QString& sourceFileName, QTextStream& destination, bool prefixWithArenaId, bool hasHeaders, bool writeHeaders) const;

signals:
	void itemChanged(ArenaItem*);

private slots:
	void jobStarted(Job* job);
	void jobFinished(Job* job);
	void jobError(Job* job);

private:
	FileItem* parentItem;

	unsigned int left;
	unsigned int top;
	unsigned int width;
	unsigned int height;

	float diameter;

	std::string firstSex;
	std::string firstGenotype;
	std::string secondSex;
	std::string secondGenotype;

	unsigned int startTime;	// seconds into the video where tracking starts
	unsigned int endTime;	// seconds into the video where tracking ends

	bool approved;

	std::string comment;

	std::string id;
	VideoStage currentVideoStage;
	Status currentVideoStatus;
	AudioStage currentAudioStage;
	Status currentAudioStatus;

	// tracking results
	double courtship;
	double quality;
	QPixmap firstEthogram;
	QPixmap secondEthogram;

	QString intSecondsToTimeString(const unsigned int t) const;

	void registerSettings();
	Settings arenaInfo;

	// helper functions for creating a Job
	QStringList getArguments(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const;
	Job* createJob(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const;	// the caller takes ownership of the returned Job
	void queueJob(Job* jobToQueue) const;
	bool canUseCluster() const;
};

#endif
