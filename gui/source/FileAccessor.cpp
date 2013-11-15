#include "FileAccessor.hpp"
#include <QFileInfo>
#include "FileItem.hpp"
#include "ArenaItem.hpp"

FileAccessor::FileAccessor() : Accessor<Item>()
{
}

Qt::ItemFlags FileAccessor::flags(const Item* item) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FileAccessor::data(const Item* item, int role) const
{
	if (role == Qt::DisplayRole) {
		return QFileInfo(item->getFileName()).fileName();	// strip path
	} else if (role == Qt::EditRole || role == Qt::ToolTipRole) {
		return item->getFileName();
	} else if (role == Qt::BackgroundRole) {
		return this->getBackgroundColor(item);
	} else {
		return QVariant();
	}
}

bool FileAccessor::setData(Item* item, const QVariant& value, int role) const
{
	return false;
}
