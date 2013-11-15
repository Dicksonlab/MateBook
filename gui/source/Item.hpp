#ifndef Item_hpp
#define Item_hpp

#include <QList>
#include <QVariant>
#include <QPixmap>
#include <QTime>
#include <QDir>
#include <QString>
#include <QModelIndex>

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "Video.hpp"

class Job;
class MateBook;
class Project;

/**
  * @class  Item
  * @brief  base class for things we want to display as a row in an ItemTree
  */
class Item : public QObject {
	Q_OBJECT

public:
	enum VideoStage {
		VideoRecording,
		ArenaDetection,
		FlyTracking,
		StatisticalVideoAnalysis
	};

	enum AudioStage {
		AudioRecording,
		PulseDetection,
		ModifyingFile,
		StatisticalAudioAnalysis
	};

	enum Status {
		Started,
		Finished,
		Queued,
		Failed
	};

	virtual ~Item();

	virtual Item* parent() = 0;	//TODO: shouldn't this be const?
	virtual Item* child(int row) const = 0;
	virtual int childCount() const = 0;
	virtual void removeChildren(int position, int count) = 0;
	virtual int childIndex() const = 0;	// the parent's child index for this item

	virtual void removeData() = 0;

	virtual boost::shared_ptr<Video> getVideo() const = 0;

	// video information
	virtual QString getFileName() const = 0;
	virtual void setFileName(const QString& name) = 0;
	virtual std::string getId() const = 0;
	virtual unsigned int getWidth() const = 0;
	virtual unsigned int getHeight() const = 0;
	virtual unsigned int getNumFrames() const = 0;
	virtual double getFps() const = 0;

	// song information
	virtual QString getNoiseFileName() const = 0;
	virtual void setNoiseFileName(const QString& name) = 0;
	virtual unsigned int getSamples() const = 0;
	virtual unsigned int getSampleRate() const = 0;
	virtual unsigned int getPulses() const = 0;
	virtual unsigned int getSines() const = 0;
	virtual unsigned int getTrains() const = 0;
	virtual unsigned int getPulseCenters() const = 0;
	virtual QString getSongOptionId() const = 0;
	virtual std::map<QString, float> getPulseDetectionOptions() = 0;
	virtual std::map<QString, float> getSongStatisticsOptions() = 0;

	virtual unsigned int getDuration() const = 0;
	virtual QDateTime getDate() const = 0;
	virtual void setDate(const QDateTime& date) = 0;

	virtual std::string getFirstSex() const = 0;
	virtual void setFirstSex(const std::string& firstSex) = 0;
	virtual std::string getFirstGenotype() const = 0;
	virtual void setFirstGenotype(const std::string& firstGenotype) = 0;
	virtual std::string getSecondSex() const = 0;
	virtual void setSecondSex(const std::string& secondSex) = 0;
	virtual std::string getSecondGenotype() const = 0;
	virtual void setSecondGenotype(const std::string& secondGenotype) = 0;

	virtual unsigned int getStartTime() const = 0;
	virtual void setStartTime(const unsigned int time) = 0;
	virtual unsigned int getEndTime() const = 0;
	virtual void setEndTime(const unsigned int time) = 0;

	virtual std::string getExperimenter() const = 0;
	virtual void setExperimenter(const std::string& experimenter) = 0;

	virtual unsigned int getApproved() const = 0;
	virtual void setApproved(bool approved) = 0;

	virtual std::string getComment() const = 0;
	virtual void setComment(const std::string& comment) = 0;

	// tracking results
	virtual bool hasCourtship() const = 0;
	virtual double getCourtship() const = 0;
	virtual bool hasQuality() const = 0;
	virtual double getQuality() const = 0;
	virtual QPixmap getFirstEthogram() const;
	virtual QPixmap getSecondEthogram() const;

	virtual void serialize() const;

	virtual void runArenaDetection() = 0;
	virtual void runFlyTracking() = 0;
	virtual void runPostprocessor() = 0;
	virtual void runStatisticalVideoAnalysis() = 0;
	virtual void runPulseDetection() = 0;
	virtual void runStatisticalSongAnalysis() = 0;

	virtual void resetArenaDetection() = 0;
	virtual void resetFlyTracking() = 0;
	virtual void resetStatisticalVideoAnalysis() = 0;
	virtual void resetPulseDetection() = 0;
	virtual void resetStatisticalSongAnalysis() = 0;

	virtual VideoStage getCurrentVideoStage() const = 0;
	virtual Status getCurrentVideoStatus() const = 0;
	virtual AudioStage getCurrentAudioStage() const = 0;
	virtual Status getCurrentAudioStatus() const = 0;

	static Item* getItem(const QModelIndex& index);

protected:
	Item(MateBook* mateBook, Project* project);

	MateBook* getMateBook() const;
	Project* getProject() const;

signals:
	void itemChanged(Item*);

private slots:
	void childItemChanged(Item*);

private:
	MateBook* mateBook;
	Project* project;
};

#endif
