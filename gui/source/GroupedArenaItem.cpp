#include "GroupedArenaItem.hpp"
#include <QStringList>
#include <QFileInfo>
#include <QTextStream>
#include <iostream>
#include <cassert>
#include "RuntimeError.hpp"
#include "../../common/source/Singleton.hpp"
#include "ExternalJob.hpp"
#include "JobQueue.hpp"
#include "GroupItem.hpp"
#include "AbstractGroupItem.hpp"
#include "MateBook.hpp"
#include "Project.hpp"
#include "../../common/source/serialization.hpp"
#include "Item.hpp"
#include "ArenaItem.hpp"

GroupedArenaItem::GroupedArenaItem(MateBook* mateBook, Project* project, ArenaItem* referenced, GroupItem* parent) : AbstractGroupItem(mateBook, project),
	referencedItem(referenced),
	parentItem(parent)
{
	//TODO: tell referenced item that it's being grouped to this->parentItem group
}

GroupedArenaItem::~GroupedArenaItem()
{
}

//TODO: see if we can remove this method from the Item interface - this is bad OO design)
void GroupedArenaItem::appendChild(GroupedArenaItem *item)
{
	assert(false);	// a GroupedArenaItem doesn't have any children
}

//TODO: see if we can remove this method from the Item interface - this is bad OO design)
GroupedArenaItem* GroupedArenaItem::child(int row) const
{
	assert(false);	// a GroupedArenaItem doesn't have any children
	return NULL;
}

int GroupedArenaItem::childCount() const
{
	return 0;
}

AbstractGroupItem* GroupedArenaItem::parent()
{
	return parentItem;
}

void GroupedArenaItem::removeChildren(int position, int count)
{
	assert(count == 0);	// a GroupedArenaItem doesn't have any children
}

int GroupedArenaItem::childIndex() const
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

void GroupedArenaItem::removedByUser()
{
	//TODO: tell referenced item that it's being removed from this->parentItem group
}

QString GroupedArenaItem::getName() const
{
	return referencedItem->getFileName() + " / " + QString::fromStdString(referencedItem->getId());
}

Item::VideoStage GroupedArenaItem::getCurrentVideoStage() const
{
	return referencedItem->getCurrentVideoStage();
}

Item::Status GroupedArenaItem::getCurrentVideoStatus() const
{
	return referencedItem->getCurrentVideoStatus();
}

Item::AudioStage GroupedArenaItem::getCurrentAudioStage() const
{
	return referencedItem->getCurrentAudioStage();
}

Item::Status GroupedArenaItem::getCurrentAudioStatus() const
{
	return referencedItem->getCurrentAudioStatus();
}

float GroupedArenaItem::getMean(size_t attributeIndex) const
{
	QString meanFileName = "mean.tsv";
	QFile file(referencedItem->absoluteDataDirectory().filePath(meanFileName));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw RuntimeError(QObject::tr("Cannot open file %1 for reading:\n%2.").arg(meanFileName).arg(file.errorString()));
	}

	// parse TSV file
	QTextStream stream(&file);
	QString line = stream.readLine();
	QStringList split = line.split('\t');
	if (split.size() <= attributeIndex) {
		throw RuntimeError(QObject::tr("File %1 does not contain enough values.").arg(meanFileName));
	}
	QString meanString = split[attributeIndex];
	bool conversionSuccessful = false;
	float mean = meanString.toFloat(&conversionSuccessful);
	if (!conversionSuccessful) {
		throw RuntimeError(QObject::tr("Value in file %1 is not a floating point number.").arg(meanFileName));
	}
	return mean;
}
