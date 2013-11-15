#ifndef FilePathAccessor_hpp
#define FilePathAccessor_hpp

#include "Accessor.hpp"
#include "ArenaItem.hpp"

/**
  * @class  DateAccessor
  * 
  */
template<class GETTER, class SETTER>
class FilePathAccessor : public Accessor<Item> {
public:
	FilePathAccessor(GETTER getter, SETTER setter) : Accessor<Item>(),
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
		if (role == Qt::DisplayRole || role == Qt::EditRole) {
			if (const ArenaItem* arenaItem = dynamic_cast<const ArenaItem*>(item)) {
				return QVariant();
			}
			return QFileInfo((item->*getter)()).absolutePath();
		} else if (role == Qt::BackgroundRole) {
			return this->getBackgroundColor(item);
		} else {
			return QVariant();
		}
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		if(role == Qt::EditRole){
				(item->*setter)(value.toString() + '/' + QFileInfo(item->getFileName()).fileName());
				return true;
		}
		return false;
	}

private:
	GETTER getter;
	SETTER setter;
};

#endif
