#ifndef GroupedArenaItem_hpp
#define GroupedArenaItem_hpp

#include <QList>
#include <QVariant>
#include <QPixmap>
#include <QTime>
#include <QDir>

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "AbstractGroupItem.hpp"
#include "Video.hpp"
#include "TrackingResults.hpp"
#include "../../common/source/Settings.hpp"

class Job;
class GroupItem;
class ArenaItem;
class MateBook;
class Project;

class GroupedArenaItem : public AbstractGroupItem {
	Q_OBJECT

public:
	GroupedArenaItem(MateBook* mateBook, Project* project, ArenaItem* referenced, GroupItem* parent);

	~GroupedArenaItem();

	AbstractGroupItem* parent();
	void appendChild(GroupedArenaItem* child);
	GroupedArenaItem* child(int row) const;
	int childCount() const;
	void removeChildren(int position, int count);
	int childIndex() const;	// the parent's child index for this item

	void removedByUser();

	QString getName() const;
	Item::VideoStage getCurrentVideoStage() const;
	Item::Status getCurrentVideoStatus() const;
	Item::AudioStage getCurrentAudioStage() const;
	Item::Status getCurrentAudioStatus() const;

	float getMean(size_t attributeIndex) const;

signals:
	void itemChanged(GroupedArenaItem*);

private:
	ArenaItem* referencedItem;
	GroupItem* parentItem;
};

#endif
