#include "ArenasDetectedAccessor.hpp"
#include "FileItem.hpp"
#include "ArenaItem.hpp"

ArenasDetectedAccessor::ArenasDetectedAccessor() : Accessor<Item>()
{
}

Qt::ItemFlags ArenasDetectedAccessor::flags(const Item* item) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ArenasDetectedAccessor::data(const Item* item, int role) const
{
	if (role == Qt::DisplayRole) {
		if (const FileItem* fileItem = dynamic_cast<const FileItem*>(item)) {
			return QString::number(fileItem->childCount());
		}
	} else if (role == Qt::BackgroundRole) {
		return this->getBackgroundColor(item);
	}
	
	return QVariant();
}

bool ArenasDetectedAccessor::setData(Item* item, const QVariant& value, int role) const
{
	return false;
}
