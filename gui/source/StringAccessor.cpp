#include "StringAccessor.hpp"

StringAccessor::StringAccessor(Getter getter, Setter setter) : Accessor<Item>(),
	getter(getter),
	setter(setter)
{
}

Qt::ItemFlags StringAccessor::flags(const Item* item) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant StringAccessor::data(const Item* item, int role) const
{
	if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
		return QString::fromStdString((item->*getter)());
	} else if (role == Qt::BackgroundRole) {
		return this->getBackgroundColor(item);
	}
	return QVariant();
}

bool StringAccessor::setData(Item* item, const QVariant& value, int role) const
{
	if (role == Qt::EditRole) {
		(item->*setter)(value.toString().toStdString());
		return true;
	}
	return false;
}
