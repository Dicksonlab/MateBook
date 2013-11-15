#include "Item.hpp"
#include <QStringList>
#include <QFileInfo>
#include <cassert>
#include "RuntimeError.hpp"
#include "JobQueue.hpp"

Item::Item(MateBook* mateBook, Project* project) :
	mateBook(mateBook),
	project(project)
{
}

Item::~Item()
{
}

QPixmap Item::getFirstEthogram() const
{
	return QPixmap();
}

QPixmap Item::getSecondEthogram() const
{
	return QPixmap();
}

void Item::serialize() const
{
}

MateBook* Item::getMateBook() const
{
	return mateBook;
}

Project* Item::getProject() const
{
	return project;
}

void Item::childItemChanged(Item*)
{
	//TODO: see if this item needs to be changed
	emit itemChanged(this);
}

Item* Item::getItem(const QModelIndex& index)
{
	if (!index.isValid()) {
		throw std::runtime_error("invalid index");
	}
//	return static_cast<Item*>(proxyModel->mapToSource(index).internalPointer());	//TODO: remove this if the code below works
	// here we assume that no proxy remaps column 0 and that it contains the address of the Item
	return static_cast<Item*>(index.sibling(index.row(), 0).data(Qt::UserRole).value<void*>());
}