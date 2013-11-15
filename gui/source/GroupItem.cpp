#include "GroupItem.hpp"
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
#include "ExternalJob.hpp"
#include "JobQueue.hpp"
#include "ArenaItem.hpp"
#include "MateBook.hpp"
#include "Project.hpp"
#include "FileUtilities2.hpp"
#include "../../common/source/serialization.hpp"
#include "GroupedArenaItem.hpp"

GroupItem::GroupItem(MateBook* mateBook, Project* project, const QString& name) : AbstractGroupItem(mateBook, project),
	name(name),
	currentVideoStage(Item::VideoRecording),
	currentVideoStatus(Item::Finished),
	currentAudioStage(Item::AudioRecording),
	currentAudioStatus(Item::Finished)
{
}

GroupItem::~GroupItem()
{
	for (int i = 0; i < childItems.size(); ++i) {
		delete childItems[i];
	}
	childItems.clear();
}

void GroupItem::appendChild(GroupedArenaItem* child)
{
	childItems.push_back(child);
	connect(childItems.back(), SIGNAL(itemChanged(GroupedArenaItem*)), this, SLOT(childItemChanged(GroupedArenaItem*)));
}

AbstractGroupItem* GroupItem::child(int row) const
{
	return childItems[row];
}

int GroupItem::childCount() const
{
	return childItems.size();
}

GroupItem* GroupItem::parent()
{
	return NULL;
}

void GroupItem::removeChildren(int position, int count)
{
	for (std::vector<GroupedArenaItem*>::const_iterator iter = childItems.begin() + position; iter != childItems.begin() + position + count; ++iter) {
		(*iter)->removedByUser();
		delete *iter;
	}
	childItems.erase(childItems.begin() + position, childItems.begin() + position + count);
}

int GroupItem::childIndex() const
{
	assert(false);	//TODO: make this a logic_error: a group item doesn't have a parent
	return -1;
}

void GroupItem::removedByUser()
{
	//TODO: tell children that they are being removed from this group
}

QString GroupItem::getName() const
{
	return name;
}

Item::VideoStage GroupItem::getCurrentVideoStage() const
{
	return currentVideoStage;
}

Item::Status GroupItem::getCurrentVideoStatus() const
{
	return currentVideoStatus;
}

Item::AudioStage GroupItem::getCurrentAudioStage() const
{
	return currentAudioStage;
}

Item::Status GroupItem::getCurrentAudioStatus() const
{
	return currentAudioStatus;
}

std::vector<float> GroupItem::getMeans(size_t attributeIndex) const
{
	std::vector<float> ret;
	for (std::vector<GroupedArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		if ((*iter)->getCurrentVideoStage() == Item::StatisticalVideoAnalysis && (*iter)->getCurrentVideoStatus() == Item::Finished) {
			ret.push_back((*iter)->getMean(attributeIndex));
		}
	}
	return ret;
}

void GroupItem::updateStateFromChildren()
{
	Item::VideoStage newStage = Item::StatisticalVideoAnalysis;
	Item::Status newStatus = Item::Finished;

	// we want do display the earliest state of any child so that we can see if a group is ready to run statistical tests
	for (std::vector<GroupedArenaItem*>::iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
		Item::VideoStage childStage = (*iter)->getCurrentVideoStage();
		Item::Status childStatus = (*iter)->getCurrentVideoStatus();
		if (childStage < newStage) {
			newStage = childStage;
			newStatus = childStatus;
		} else if (childStage == newStage && childStatus < newStatus) {
			newStatus = childStatus;
		}
	}

	if (newStage != currentVideoStage || newStatus != currentVideoStatus) {
		currentVideoStage = newStage;
		currentVideoStatus = newStatus;
	}
	emit itemChanged(this);
}

void GroupItem::childItemChanged(GroupedArenaItem* child)
{
	emit itemChanged(child);
	updateStateFromChildren();
}
