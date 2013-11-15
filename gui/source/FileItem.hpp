#ifndef FileItem_hpp
#define FileItem_hpp

#include <QList>
#include <QVariant>
#include <QPixmap>
#include <QTime>
#include <QDir>
#include <QString>

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "Item.hpp"
#include "Video.hpp"
#include "SongResults.hpp"
#include "../../common/source/Settings.hpp"

class Job;
class ArenaItem;
class Project;
class SongResults;

class FileItem : public Item {
	Q_OBJECT

public:
	FileItem(MateBook* mateBook, Project* project, const QDir& relativeDataDirectory, FileItem* parent = 0);	// for when the data directory exists
	FileItem(MateBook* mateBook, Project* project, const QString& fileName, FileItem* parent = 0);	// for when a new data directory is to be created

	~FileItem();

	FileItem* parent();
	void appendChild(ArenaItem* child);
	Item* child(int row) const;
	int childCount() const;
	void removeChildren(int position, int count);
	int childIndex() const;	// the parent's child index for this item

	void removeData();

	boost::shared_ptr<Video> getVideo() const;
	SongResults getSongResults() const;

	// video information
	QString getFileName() const;
	void setFileName(const QString& name);
	std::string getId() const;
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

	FileItem::VideoStage getCurrentVideoStage() const;
	FileItem::Status getCurrentVideoStatus() const;
	FileItem::AudioStage getCurrentAudioStage() const;
	FileItem::Status getCurrentAudioStatus() const;

	void updateStateFromFiles();
	void updateStateFromChildren();

	const std::vector<ArenaItem*>& getChildItems() const;	// FileItem-specific

	void createArena(QRect boundingBox);

signals:
//	void itemChanged(Item*);	//TODO: can this be removed since it's anyway defined in Item?
	void beginInsertChildren(Item* thisItem, int first, int last);
	void endInsertChildren(Item* thisItem, int first, int last);
	void beginRemoveChildren(Item* thisItem, int first, int last);
	void endRemoveChildren(Item* thisItem, int first, int last);

public slots:
	void updateSongData(const SongResults& results);

private slots:
	void jobStarted(Job* job);
	void jobFinished(Job* job);
	void jobError(Job* job);

	void childItemChanged(ArenaItem* child);
	void startPulseDetection();

private:
	void removeChildrenKeepData(int position, int count);

	FileItem* parentItem;
	std::vector<ArenaItem*> childItems;

	QDir dataDirectory;
	mutable boost::weak_ptr<Video> video;

	// video information (cached here so we don't have to keep the Video open, which would require a lot of memory)
	std::string videoFilePath;
	unsigned int width;
	unsigned int height;
	unsigned int numFrames;
	double fps;

	// song information
	std::string songOptionId;
	std::string noiseFileName;
	unsigned int samples;
	unsigned int sampleRate;
	unsigned int pulses;
	unsigned int sines;
	unsigned int trains;
	unsigned int pulseCenters;

	QDateTime fileDate;

	std::string firstSex;
	std::string firstGenotype;
	std::string secondSex;
	std::string secondGenotype;

	unsigned int startTime;	// seconds into the video where tracking starts
	unsigned int endTime;	// seconds into the video where tracking ends

	std::string experimenter;

	std::string comment;

	VideoStage currentVideoStage;
	Status currentVideoStatus;
	AudioStage currentAudioStage;
	Status currentAudioStatus;

	QString intSecondsToTimeString(const unsigned int t) const;
	std::map<QString, float> pulseDetectionOptions;

	void registerSettings();
	Settings videoInfo;

	// helper functions for creating a Job
	QStringList getArguments(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const;
	Job* createJob(const QString& settingsFileName, bool preprocess, bool track, bool postprocess) const;	// the caller takes ownership of the returned Job
	void queueJob(Job* jobToQueue) const;
	bool canUseCluster() const;
};

#endif
