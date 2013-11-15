#ifndef TimeOfDayAccessor_hpp
#define TimeOfDayAccessor_hpp

#include "Accessor.hpp"
#include "ArenaItem.hpp"

template<class GETTER, class SETTER>
class TimeOfDayAccessor : public Accessor<Item> {
public:
	TimeOfDayAccessor(GETTER getter, SETTER setter) : Accessor<Item>(),
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
			return (item->*getter)().toString("hh:mm:ss");
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
			QTime time = value.toTime();
			if(!time.isValid()){
				return false;
			}
			QDateTime dateTime = (item->*getter)();
			dateTime.setTime(time);
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
