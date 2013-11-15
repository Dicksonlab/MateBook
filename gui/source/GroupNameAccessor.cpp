#include "GroupNameAccessor.hpp"
#include "AbstractGroupItem.hpp"

GroupNameAccessor::GroupNameAccessor() : Accessor<AbstractGroupItem>()
{
}

Qt::ItemFlags GroupNameAccessor::flags(const AbstractGroupItem* item) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant GroupNameAccessor::data(const AbstractGroupItem* item, int role) const
{
	if (role == Qt::BackgroundRole) {
		return this->getBackgroundColor(item);
	}
	if (role != Qt::DisplayRole && role != Qt::EditRole) {
		return QVariant();
	}
	return item->getName();
}

bool GroupNameAccessor::setData(AbstractGroupItem* item, const QVariant& value, int role) const
{
	return false;
}
