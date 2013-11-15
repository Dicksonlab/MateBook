#include "ArenasApprovedAccessor.hpp"
#include "FileItem.hpp"
#include "ArenaItem.hpp"

ArenasApprovedAccessor::ArenasApprovedAccessor() : Accessor<Item>()
{
}

Qt::ItemFlags ArenasApprovedAccessor::flags(const Item* item) const
{
	// not editable so one can click the checkbox directly
	//if (dynamic_cast<const ArenaItem*>(item)) {
	//	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	//}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ArenasApprovedAccessor::data(const Item* item, int role) const
{
	if (role == Qt::DisplayRole) {
		if(item->childCount() == 0){
			return item->getApproved() * 2;
		}
		if(item->getApproved() == 0){
			return 0;
		}
		if(item->childCount() == item->getApproved()){
			return 2;
		}
		return 1;
	} else if (role == Qt::BackgroundRole) {
		return this->getBackgroundColor(item);
	}
	
	return QVariant();
}

bool ArenasApprovedAccessor::setData(Item* item, const QVariant& value, int role) const
{
	if (role != Qt::EditRole) {
		return false;
	}
	if (value.canConvert<bool>()) {
		item->setApproved(value.toBool());
		return true;
	}

	return false;
}