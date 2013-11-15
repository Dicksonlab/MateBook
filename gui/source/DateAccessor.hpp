#ifndef DateAccessor_hpp
#define DateAccessor_hpp

#include "Accessor.hpp"
#include "ArenaItem.hpp"

template<class GETTER, class SETTER>
class DateAccessor : public Accessor<Item> {
public:
	DateAccessor(GETTER getter, SETTER setter) : Accessor<Item>(),
		getter(getter),
		setter(setter)
	{
	}

	Qt::ItemFlags flags(const Item* item) const
	{
		if (const ArenaItem* arenaItem = dynamic_cast<const ArenaItem*>(item)) {
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		}else{
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
		}
	}

	QVariant data(const Item* item, int role) const
	{
		if (role == Qt::DisplayRole) {
			return (item->*getter)().toString("yyyy-MM-dd");
		} else if (role == Qt::EditRole || role == Qt::ToolTipRole) {
			return (item->*getter)();
		} else if (role == Qt::BackgroundRole) {
			return this->getBackgroundColor(item);
		} else {
			return QVariant();
		}
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		if (role == Qt::EditRole) {
			QDate date = value.toDate();
			if(!date.isValid()){
				return false;
			}
			QDateTime dateTime = (item->*getter)();
			dateTime.setDate(date);
			(item->*setter)(dateTime);
			return true;
		}
		return false;
	}

private:
	GETTER getter;
	SETTER setter;
};

#endif
