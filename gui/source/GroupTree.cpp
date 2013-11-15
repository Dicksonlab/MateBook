#include <stdexcept>

#include "GroupTree.hpp"
#include "GroupItem.hpp"
#include "AddressAccessor.hpp"
#include "ArenasAnalyzedAccessor.hpp"
#include "ArenasApprovedAccessor.hpp"
#include "ArenasDetectedAccessor.hpp"
#include "ReadAccessor.hpp"
#include "FileAccessor.hpp"
#include "VideoStageAccessor.hpp"
#include "AudioStageAccessor.hpp"
#include "VideoStatusAccessor.hpp"
#include "AudioStatusAccessor.hpp"
#include "ReadTimeAccessor.hpp"
#include "FilePathAccessor.hpp"
#include "TimeAccessor.hpp"
#include "DateAccessor.hpp"
#include "ReadWriteAccessor.hpp"
#include "GroupNameAccessor.hpp"
#include "../../common/source/debug.hpp"
#include "Project.hpp"
#include "GroupedArenaItem.hpp"

#include "../../mediawrapper/source/mediawrapper.hpp"

#include <QHeaderView>
#include <QMessageBox>
#include <QTime>
#include <QDateTime>
#include <memory>
#include <cassert>

GroupTree::GroupTree(MateBook* mateBook, Project* project, QObject* parent) : QAbstractItemModel(parent),
	mateBook(mateBook),
	project(project)
{
	addHeader("Address", new AddressAccessor<AbstractGroupItem>());	// for debugging and to get an AbstractGroupItem* through various proxy models
	addHeader("Name", new GroupNameAccessor());
	addHeader("Video Stage", new VideoStageAccessor<AbstractGroupItem>());
	addHeader("Video Status", new VideoStatusAccessor<AbstractGroupItem>());
	addHeader("Audio Stage", new AudioStageAccessor<AbstractGroupItem>());
	addHeader("Audio Status", new AudioStatusAccessor<AbstractGroupItem>());
}

GroupTree::~GroupTree()
{
	clear();

	for (std::vector<Accessor<AbstractGroupItem>*>::const_iterator iter = accessors.begin(); iter != accessors.end(); ++iter) {
		delete *iter;
	}
}

void GroupTree::clear()
{
	if (groups.size() == 0) {
		return;
	}
	beginRemoveRows(QModelIndex(), 0, groups.size() - 1);
	for (std::vector<AbstractGroupItem*>::const_iterator iter = groups.begin(); iter != groups.end(); ++iter) {
		delete *iter;
	}
	groups.clear();
	groupIndexes.clear();
	endRemoveRows();
}

int GroupTree::columnCount(const QModelIndex& parent) const
{
	return headers.size();
}

QVariant GroupTree::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	AbstractGroupItem* item = getItem(index);
	Accessor<AbstractGroupItem>* accessor = accessors[index.column()];
	return accessor->data(item, role);
}

bool GroupTree::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid()) {
		return false;
	}

	AbstractGroupItem* item = getItem(index);
	Accessor<AbstractGroupItem>* accessor = accessors[index.column()];
	bool success = false;
	try {
		success = accessor->setData(item, value, role);
	} catch (std::runtime_error& e) {
		QMessageBox::warning(0, tr("Setting %1").arg(headers[index.column()]), tr(e.what()));
	}
	if (success) {
		emit dataChanged(index, index);	//TODO: for some properties we'll have to signal a data change for the entire row
	}
	return success;
}

Qt::ItemFlags GroupTree::flags(const QModelIndex& index) const
{
	if (!index.isValid()) {
		return 0;
	}

	AbstractGroupItem* item = getItem(index);
	Accessor<AbstractGroupItem>* accessor = accessors[index.column()];
	return accessor->flags(item);
}

QVariant GroupTree::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		return headers[section];
	}
	return QVariant();
}

bool GroupTree::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
	if (role == Qt::EditRole && orientation == Qt::Horizontal && value.canConvert<QString>()) {
		QString text = value.toString();

		// make sure there's no other (!) header with that text
		std::map<QString, int>::const_iterator iter = headerToColumn.find(text);
		if (iter != headerToColumn.end() && iter->second != section) {
			return false;
		}

		headerToColumn.erase(headers[section]);
		headerToColumn[text] = section;
		headers[section] = text;
		emit headerDataChanged(orientation, section, section);
		return true;
	}
	return false;
}

QModelIndex GroupTree::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (!parent.isValid()) {
		return createIndex(row, column, groups[row]);
	}

	AbstractGroupItem* parentItem = static_cast<AbstractGroupItem*>(parent.internalPointer());
	AbstractGroupItem* childItem = parentItem->child(row);
	if (childItem) {
		return createIndex(row, column, childItem);
	}

	return QModelIndex();
}

QModelIndex GroupTree::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	AbstractGroupItem* childItem = static_cast<AbstractGroupItem*>(index.internalPointer());
	AbstractGroupItem* parentItem = childItem->parent();
	if (!parentItem) {
		return QModelIndex();
	}

	return createIndex(getRow(parentItem), 0, parentItem);
}

int GroupTree::rowCount(const QModelIndex& parent) const
{
	// TODO: is this check necessary?
	if (parent.column() > 0) {
		return 0;
	}

	if (!parent.isValid()) {
		return groups.size();
	}

	AbstractGroupItem* parentItem = static_cast<AbstractGroupItem*>(parent.internalPointer());
	return parentItem->childCount();
}
/*
bool GroupTree::insertRows(int position, int rows, const QModelIndex& parent)
{
	if (!parent.isValid()) {
		beginInsertRows(parent, position, position + rows - 1);
		groups.insert(groups.begin() + position, rows, NULL);
		for (std::vector<AbstractGroupItem*>::iterator iter = groups.begin() + position; iter != groups.begin() + position + rows; ++iter) {
			*iter = new AbstractGroupItem;
		}
		endInsertRows();
		return true;
	}
	return false;
}
*/
bool GroupTree::removeRows(int position, int count, const QModelIndex& parent)
{
	if (!parent.isValid()) {
		beginRemoveRows(parent, position, position + count - 1);
		for (std::vector<AbstractGroupItem*>::const_iterator iter = groups.begin() + position; iter != groups.begin() + position + count; ++iter) {
			(*iter)->removedByUser();
			delete *iter;
		}
		groups.erase(groups.begin() + position, groups.begin() + position + count);
		endRemoveRows();
		return true;
	}

	beginRemoveRows(parent, position, position + count - 1);
	getItem(parent)->removeChildren(position, count);
	endRemoveRows();
	return true;
}
/*
bool GroupTree::insertColumns(int position, int columns, const QModelIndex& parent)
{
	beginInsertColumns(parent, position, position + columns - 1);
	bool success = rootItem->insertColumns(position, columns);
	endInsertColumns();
	return success;
}

bool GroupTree::removeColumns(int position, int columns, const QModelIndex& parent)
{
	beginRemoveColumns(parent, position, position + columns - 1);
	bool success = rootItem->removeColumns(position, columns);
	endRemoveColumns();
	if (rootItem->columnCount() == 0) {
		removeRows(0, rowCount());
	}
	return success;
}
*/

bool GroupTree::addArenaItemToGroup(ArenaItem* arenaItem, const QString& groupName)
{
	emit layoutAboutToBeChanged();

	std::map<QString, size_t>::iterator iter = groupIndexes.find(groupName);
	if (iter == groupIndexes.end()) {
		groupIndexes[groupName] = groups.size();
		groups.push_back(new GroupItem(mateBook, project, groupName));
		connect(groups.back(), SIGNAL(itemChanged(AbstractGroupItem*)), this, SLOT(itemChanged(AbstractGroupItem*)));
	}

	GroupItem* groupItem = assert_cast<GroupItem*>(groups[groupIndexes[groupName]]);
	groupItem->appendChild(new GroupedArenaItem(mateBook, project, arenaItem, groupItem));

	emit layoutChanged();
	emit dataChanged(createIndex(rowCount() - 1, 0, groups.back()), createIndex(rowCount() - 1, columnCount() - 1, groups.back()));
	return true;
}

void GroupTree::addHeader(const QString& header, Accessor<AbstractGroupItem>* accessor)
{
	headerToColumn[header] = headers.size();
	headers.push_back(header);

	accessors.push_back(accessor);

	if (!groups.empty()) {
		emit dataChanged(createIndex(0, 0, groups.front()), createIndex(rowCount() - 1, columnCount() - 1, groups.back()));
	}
	emit layoutChanged();
}

AbstractGroupItem* GroupTree::getItem(const QModelIndex& index) const
{
	if (!index.isValid()) {
		throw std::runtime_error("invalid index");
	}
	return static_cast<AbstractGroupItem*>(index.internalPointer());
}

int GroupTree::getColumn(QString header) const
{
	std::map<QString, int>::const_iterator iter = headerToColumn.find(header);
	if (iter == headerToColumn.end()) {
		throw std::runtime_error("header not found");
	}
	return iter->second;
}

int GroupTree::getRow(AbstractGroupItem* item) const
{
	if (item->parent()) {
		return item->childIndex();
	}
	
	// it's a top-level item
	std::vector<AbstractGroupItem*>::const_iterator parentIter = std::find(groups.begin(), groups.end(), item);
	assert(parentIter != groups.end());	// it has to be in the list
	return parentIter - groups.begin();
}

const std::vector<AbstractGroupItem*>& GroupTree::getGroups() const
{
	return groups;
}

const std::map<QString, size_t>& GroupTree::getGroupIndexes() const
{
	return groupIndexes;
}

void GroupTree::itemChanged(AbstractGroupItem* item)
{
	int row = getRow(item);
	emit dataChanged(createIndex(row, 0, item), createIndex(row, columnCount() - 1, item));
}
