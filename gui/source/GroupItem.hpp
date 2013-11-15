#ifndef GroupItem_hpp
#define GroupItem_hpp

#include <QList>
#include <QVariant>
#include <QPixmap>
#include <QTime>
#include <QDir>

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "AbstractGroupItem.hpp"
#include "../../common/source/Settings.hpp"

class Job;
class GroupedArenaItem;
class Project;
class SongResults;

class GroupItem : public AbstractGroupItem {
	Q_OBJECT

public:
	GroupItem(MateBook* mateBook, Project* project, const QString& name);

	~GroupItem();

	GroupItem* parent();
	void appendChild(GroupedArenaItem* child);
	AbstractGroupItem* child(int row) const;
	int childCount() const;
	void removeChildren(int position, int count);
	int childIndex() const;	// the parent's child index for this item

	void removedByUser();

	QString getName() const;
	Item::VideoStage getCurrentVideoStage() const;
	Item::Status getCurrentVideoStatus() const;
	Item::AudioStage getCurrentAudioStage() const;
	Item::Status getCurrentAudioStatus() const;

	std::vector<float> getMeans(size_t attributeIndex) const;

	void updateStateFromChildren();

signals:
//TODO: can this be removed since it's anyway defined in Item?
//	void itemChanged(Item*);

public slots:
	void childItemChanged(GroupedArenaItem* child);

private:
	std::vector<GroupedArenaItem*> childItems;

	QString name;
	Item::VideoStage currentVideoStage;
	Item::Status currentVideoStatus;
	Item::AudioStage currentAudioStage;
	Item::Status currentAudioStatus;

	QString intSecondsToTimeString(const unsigned int t) const;
	std::map<QString, float> pulseDetectionOptions;
};

#endif
