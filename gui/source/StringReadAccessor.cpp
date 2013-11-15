#include "StringReadAccessor.hpp"

StringReadAccessor::StringReadAccessor(Getter getter) : Accessor<Item>(),
	getter(getter)
{
}

Qt::ItemFlags StringReadAccessor::flags(const Item* item) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant StringReadAccessor::data(const Item* item, int role) const
{
	if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
		return QString::fromStdString((item->*getter)());
	} else if (role == Qt::BackgroundRole) {
		return this->getBackgroundColor(item);
	}
	return QVariant();
}

bool StringReadAccessor::setData(Item* item, const QVariant& value, int role) const
{
	return false;
}
