#include "AbstractGroupItem.hpp"
#include <QStringList>
#include <QFileInfo>
#include <cassert>
#include "RuntimeError.hpp"
#include "JobQueue.hpp"

AbstractGroupItem::AbstractGroupItem(MateBook* mateBook, Project* project) :
	mateBook(mateBook),
	project(project)
{
}

AbstractGroupItem::~AbstractGroupItem()
{
}

MateBook* AbstractGroupItem::getMateBook() const
{
	return mateBook;
}

Project* AbstractGroupItem::getProject() const
{
	return project;
}

void AbstractGroupItem::childItemChanged(AbstractGroupItem*)
{
	//TODO: see if this item needs to be changed
	emit itemChanged(this);
}
