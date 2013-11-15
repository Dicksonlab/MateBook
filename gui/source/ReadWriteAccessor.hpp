#ifndef ReadWriteAccessor_hpp
#define ReadWriteAccessor_hpp

#include "Accessor.hpp"
#include "ArenaItem.hpp"

/**
  * @class  FilePathNameAccessor
  * 
  */
template<class GETTER, class SETTER>
class ReadWriteAccessor : public Accessor<Item> {
public:
	ReadWriteAccessor(GETTER getter, SETTER setter) : Accessor<Item>(),
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
			return (item->*getter)();
		} else if (role == Qt::BackgroundRole) {
			return this->getBackgroundColor(item);
		} else {
			return QVariant();
		}
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		if(role == Qt::EditRole){
				(item->*setter)(value.toString());
				return true;
		}
		return false;
	}

private:
	GETTER getter;
	SETTER setter;
};

#endif
