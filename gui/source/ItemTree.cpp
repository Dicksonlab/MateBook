#include <stdexcept>

#include "ItemTree.hpp"
#include "FileItem.hpp"
#include "AddressAccessor.hpp"
#include "ArenasAnalyzedAccessor.hpp"
#include "ArenasApprovedAccessor.hpp"
#include "ArenasDetectedAccessor.hpp"
#include "StringAccessor.hpp"
#include "StringReadAccessor.hpp"
#include "DefinedReadAccessor.hpp"
#include "ReadAccessor.hpp"
#include "FileAccessor.hpp"
#include "VideoStageAccessor.hpp"
#include "VideoStatusAccessor.hpp"
#include "AudioStageAccessor.hpp"
#include "AudioStatusAccessor.hpp"
#include "ReadTimeAccessor.hpp"
#include "FilePathAccessor.hpp"
#include "TimeAccessor.hpp"
#include "DateAccessor.hpp"
#include "TimeOfDayAccessor.hpp"
#include "ReadWriteAccessor.hpp"
#include "ImageAccessor.hpp"
#include "../../common/source/ScopeGuard.hpp"
#include "Project.hpp"
#include "MateBook.hpp"

#include "../../mediawrapper/source/mediawrapper.hpp"

#include <QHeaderView>
#include <QMessageBox>
#include <QTime>
#include <QDateTime>
#include <QReadLocker>
#include <QWriteLocker>
#include <QProgressDialog>
#include <memory>
#include <cassert>

ItemTree::ItemTree(MateBook* mateBook, Project* project, QObject* parent) : QAbstractItemModel(parent),
	mateBook(mateBook),
	project(project)
{
	clear();
	ScopeGuard guard = makeGuard(&ItemTree::clear, this);

	QDir projectDir = project->getDirectory();
	QStringList videoDirs = projectDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

	QProgressDialog progress("Opening project...", "Abort", 0, videoDirs.size(), mateBook);
	progress.setWindowModality(Qt::WindowModal);
	for (QStringList::const_iterator iter = videoDirs.constBegin(); iter != videoDirs.constEnd(); ++iter) {
		files.push_back(new FileItem(mateBook, project, QDir(*iter)));
		connect(files.back(), SIGNAL(itemChanged(Item*)), this, SLOT(itemChanged(Item*)));
		connect(files.back(), SIGNAL(beginInsertChildren(Item*, int, int)), this, SLOT(beginInsertChildren(Item*, int, int)));
		connect(files.back(), SIGNAL(endInsertChildren(Item*, int, int)), this, SLOT(endInsertChildren(Item*, int, int)));
		connect(files.back(), SIGNAL(beginRemoveChildren(Item*, int, int)), this, SLOT(beginRemoveChildren(Item*, int, int)));
		connect(files.back(), SIGNAL(endRemoveChildren(Item*, int, int)), this, SLOT(endRemoveChildren(Item*, int, int)));

		progress.setValue(progress.value() + 1);
		if (progress.wasCanceled()) {
			break;
		}
	}

	// address: for debugging and to get an Item* through various proxy models
	addHeader("Address", new AddressAccessor<Item>());
	// more important headers
	addHeader("Path", new FilePathAccessor<QString (Item::*)() const, void (Item::*)(const QString&)>(&Item::getFileName, &Item::setFileName));
	addHeader("File", new FileAccessor());
	addHeader("ID", new StringReadAccessor(&Item::getId));
	addHeader("Start", new TimeAccessor<unsigned int (Item::*)() const,  void (Item::*)(const unsigned int)>(&Item::getStartTime, &Item::setStartTime));
	addHeader("End", new TimeAccessor<unsigned int (Item::*)() const,  void (Item::*)(const unsigned int)>(&Item::getEndTime, &Item::setEndTime));
	// stage, status
	addHeader("Video Stage", new VideoStageAccessor<Item>());
	addHeader("Video Status", new VideoStatusAccessor<Item>());
	addHeader("Courtship", new DefinedReadAccessor<double, bool (Item::*)() const, double (Item::*)() const>(&Item::hasCourtship, &Item::getCourtship));
	addHeader("Quality", new DefinedReadAccessor<double, bool (Item::*)() const, double (Item::*)() const>(&Item::hasQuality, &Item::getQuality));
	// video specific
	addHeader("Arenas detected", new ArenasDetectedAccessor());
	addHeader("Arenas analyzed", new ArenasAnalyzedAccessor());
	addHeader("Arenas approved", new ArenasApprovedAccessor());
	addHeader("Width", new ReadAccessor<unsigned int, unsigned int (Item::*)() const>(&Item::getWidth));
	addHeader("Height", new ReadAccessor<unsigned int, unsigned int (Item::*)() const>(&Item::getHeight));
	addHeader("Frames", new ReadAccessor<unsigned int, unsigned int (Item::*)() const>(&Item::getNumFrames));
	addHeader("FPS", new ReadAccessor<double, double (Item::*)() const>(&Item::getFps));
	// audio specific
	addHeader("Audio Stage", new AudioStageAccessor<Item>());
	addHeader("Audio Status", new AudioStatusAccessor<Item>());
	addHeader("Noise File", new ReadWriteAccessor<QString (Item::*)() const, void (Item::*)(const QString&)>(&Item::getNoiseFileName, &Item::setNoiseFileName));
	addHeader("Samples", new ReadAccessor<unsigned int, unsigned int (Item::*)() const>(&Item::getSamples));
	addHeader("Sample rate", new ReadAccessor<unsigned int, unsigned int (Item::*)() const>(&Item::getSampleRate));
	addHeader("Pulses detected", new ReadAccessor<unsigned int, unsigned int (Item::*) () const>(&Item::getPulses));
	addHeader("Pulse Centers detected", new ReadAccessor<unsigned int, unsigned int (Item::*) () const>(&Item::getPulseCenters));
	addHeader("Sines detected", new ReadAccessor<unsigned int, unsigned int (Item::*) () const>(&Item::getSines));
	addHeader("Trains analyzed", new ReadAccessor<unsigned int, unsigned int (Item::*) () const>(&Item::getTrains));
	// common headers
	addHeader("Duration", new ReadTimeAccessor<unsigned int, unsigned int (Item::*)() const>(&Item::getDuration));
	addHeader("Experimenter", new StringAccessor(&Item::getExperimenter, &Item::setExperimenter));
	addHeader("Date", new DateAccessor<QDateTime (Item::*)() const, void (Item::*)(const QDateTime&)>(&Item::getDate, &Item::setDate));
	addHeader("Time of Day", new TimeOfDayAccessor<QDateTime (Item::*)() const, void (Item::*)(const QDateTime&)>(&Item::getDate, &Item::setDate));
	addHeader("1st sex", new StringAccessor(&Item::getFirstSex, &Item::setFirstSex));
	addHeader("1st genotype", new StringAccessor(&Item::getFirstGenotype, &Item::setFirstGenotype));
	addHeader("1st behavior", new ImageAccessor<QPixmap, QPixmap (Item::*) () const>(&Item::getFirstEthogram));
	addHeader("2nd sex", new StringAccessor(&Item::getSecondSex, &Item::setSecondSex));
	addHeader("2nd genotype", new StringAccessor(&Item::getSecondGenotype, &Item::setSecondGenotype));
	addHeader("2nd behavior", new ImageAccessor<QPixmap, QPixmap (Item::*) () const>(&Item::getSecondEthogram));
	addHeader("Comment", new StringAccessor(&Item::getComment, &Item::setComment));

	guard.dismiss();
}

ItemTree::~ItemTree()
{
	clear();
}

void ItemTree::clear()
{
	for (std::vector<Item*>::const_iterator iter = files.begin(); iter != files.end(); ++iter) {
		delete *iter;
	}

	for (std::vector<Accessor<Item>*>::const_iterator iter = accessors.begin(); iter != accessors.end(); ++iter) {
		delete *iter;
	}
}

int ItemTree::columnCount(const QModelIndex& parent) const
{
	return headers.size();
}

QVariant ItemTree::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	Item* item = getItem(index);
	Accessor<Item>* accessor = accessors[index.column()];
	return accessor->data(item, role);
}

bool ItemTree::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid()) {
		return false;
	}

	Item* item = getItem(index);
	Accessor<Item>* accessor = accessors[index.column()];
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

Qt::ItemFlags ItemTree::flags(const QModelIndex& index) const
{
	if (!index.isValid()) {
		return 0;
	}

	Item* item = getItem(index);
	Accessor<Item>* accessor = accessors[index.column()];
	return accessor->flags(item);
}

QVariant ItemTree::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		return headers[section];
	}
	return QVariant();
}

bool ItemTree::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
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

QModelIndex ItemTree::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (!parent.isValid()) {
		return createIndex(row, column, files[row]);
	}

	Item* parentItem = static_cast<Item*>(parent.internalPointer());
	Item* childItem = parentItem->child(row);
	if (childItem) {
		return createIndex(row, column, childItem);
	}

	return QModelIndex();
}

QModelIndex ItemTree::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	Item* childItem = static_cast<Item*>(index.internalPointer());
	Item* parentItem = childItem->parent();
	if (!parentItem) {
		return QModelIndex();
	}

	return createIndex(getRow(parentItem), 0, parentItem);
}

int ItemTree::rowCount(const QModelIndex& parent) const
{
	// TODO: is this check necessary?
	if (parent.column() > 0) {
		return 0;
	}

	if (!parent.isValid()) {
		return files.size();
	}

	Item* parentItem = static_cast<Item*>(parent.internalPointer());
	return parentItem->childCount();
}
/*
bool ItemTree::insertRows(int position, int rows, const QModelIndex& parent)
{
	if (!parent.isValid()) {
		beginInsertRows(parent, position, position + rows - 1);
		files.insert(files.begin() + position, rows, NULL);
		for (std::vector<Item*>::iterator iter = files.begin() + position; iter != files.begin() + position + rows; ++iter) {
			*iter = new Item;
		}
		endInsertRows();
		return true;
	}
	return false;
}
*/
bool ItemTree::removeRows(int position, int count, const QModelIndex& parent)
{
	if (!parent.isValid()) {
		beginRemoveRows(parent, position, position + count - 1);
		for (std::vector<Item*>::const_iterator iter = files.begin() + position; iter != files.begin() + position + count; ++iter) {
			(*iter)->removeData();
			delete *iter;
		}
		files.erase(files.begin() + position, files.begin() + position + count);
		endRemoveRows();
		return true;
	}

	beginRemoveRows(parent, position, position + count - 1);
	getItem(parent)->removeChildren(position, count);
	endRemoveRows();
	return true;
}
/*
bool ItemTree::insertColumns(int position, int columns, const QModelIndex& parent)
{
	beginInsertColumns(parent, position, position + columns - 1);
	bool success = rootItem->insertColumns(position, columns);
	endInsertColumns();
	return success;
}

bool ItemTree::removeColumns(int position, int columns, const QModelIndex& parent)
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

void ItemTree::serialize() const
{
	for (std::vector<Item*>::const_iterator iter = files.begin(); iter != files.end(); ++iter) {
		(*iter)->serialize();
	}
}

bool ItemTree::addVideo(const QString& fileName)
{
	emit layoutAboutToBeChanged();
	try {
		files.push_back(new FileItem(mateBook, project, fileName));
		connect(files.back(), SIGNAL(itemChanged(Item*)), this, SLOT(itemChanged(Item*)));
		connect(files.back(), SIGNAL(beginInsertChildren(Item*, int, int)), this, SLOT(beginInsertChildren(Item*, int, int)));
		connect(files.back(), SIGNAL(endInsertChildren(Item*, int, int)), this, SLOT(endInsertChildren(Item*, int, int)));
		connect(files.back(), SIGNAL(beginRemoveChildren(Item*, int, int)), this, SLOT(beginRemoveChildren(Item*, int, int)));
		connect(files.back(), SIGNAL(endRemoveChildren(Item*, int, int)), this, SLOT(endRemoveChildren(Item*, int, int)));
	} catch (std::runtime_error& e) {
		emit layoutChanged();
		return false;
	}
	emit layoutChanged();
	emit dataChanged(createIndex(rowCount() - 1, 0, files.back()), createIndex(rowCount() - 1, columnCount() - 1, files.back()));
	return true;
}

void ItemTree::addHeader(const QString& header, Accessor<Item>* accessor)
{
	headerToColumn[header] = headers.size();
	headers.push_back(header);
	accessors.push_back(accessor);

	if (!files.empty()) {
		emit dataChanged(createIndex(0, 0, files.front()), createIndex(rowCount() - 1, columnCount() - 1, files.back()));
	}
	emit layoutChanged();
}

Item* ItemTree::getItem(const QModelIndex& index) const
{
	if (!index.isValid()) {
		throw std::runtime_error("invalid index");
	}
	return static_cast<Item*>(index.internalPointer());
}

int ItemTree::getColumn(QString header) const
{
	std::map<QString, int>::const_iterator iter = headerToColumn.find(header);
	if (iter == headerToColumn.end()) {
		throw std::runtime_error("header not found");
	}
	return iter->second;
}

int ItemTree::getRow(Item* item) const
{
	if (item->parent()) {
		return item->childIndex();
	}
	
	// it's a top-level item
	std::vector<Item*>::const_iterator parentIter = std::find(files.begin(), files.end(), item);
	assert(parentIter != files.end());	// it has to be in the list
	return parentIter - files.begin();
}

void ItemTree::itemChanged(Item* item)
{
	int row = getRow(item);
	emit dataChanged(createIndex(row, 0, item), createIndex(row, columnCount() - 1, item));
}

void ItemTree::beginInsertChildren(Item* parent, int first, int last)
{
	emit beginInsertRows(createIndex(getRow(parent), 0, parent), first, last);
}

void ItemTree::endInsertChildren(Item* parent, int first, int last)
{
	emit endInsertRows();
}

void ItemTree::beginRemoveChildren(Item* parent, int first, int last)
{
	emit beginRemoveRows(createIndex(getRow(parent), 0, parent), first, last);
}

void ItemTree::endRemoveChildren(Item* parent, int first, int last)
{
	emit endRemoveRows();
}
