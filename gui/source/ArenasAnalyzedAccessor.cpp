#include "ArenasAnalyzedAccessor.hpp"
#include "FileItem.hpp"
#include "ArenaItem.hpp"

ArenasAnalyzedAccessor::ArenasAnalyzedAccessor() : Accessor<Item>()
{
}

Qt::ItemFlags ArenasAnalyzedAccessor::flags(const Item* item) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ArenasAnalyzedAccessor::data(const Item* item, int role) const
{
	if (role == Qt::DisplayRole) {
		if (const FileItem* fileItem = dynamic_cast<const FileItem*>(item)) {
			//TODO: for now we're showing how many arenas have been tracked
			int arenasTracked = 0;
			for (int i = 0; i != fileItem->childCount(); ++i) {
				arenasTracked += (fileItem->child(i)->getCurrentVideoStage() == Item::FlyTracking && fileItem->child(i)->getCurrentVideoStatus() == Item::Finished);
			}
			return QString::number(arenasTracked);
		}
	} else if (role == Qt::BackgroundRole) {
		return this->getBackgroundColor(item);
	}
	
	return QVariant();
}

bool ArenasAnalyzedAccessor::setData(Item* item, const QVariant& value, int role) const
{
	return false;
}
