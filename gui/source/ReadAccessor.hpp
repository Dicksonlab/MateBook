#ifndef ReadAccessor_hpp
#define ReadAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  ReadAccessor
  * @brief  to read Item properties that have a getter function
  *
  * GETTER is a Item member function returning a value convertible to a QVariant
  */
template<class T, class GETTER>
class ReadAccessor : public Accessor<Item> {
public:
	ReadAccessor(GETTER getter) : Accessor<Item>(),
		getter(getter)
	{
	}

	Qt::ItemFlags flags(const Item* item) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	QVariant data(const Item* item, int role) const
	{
		if (role == Qt::BackgroundRole) {
			return this->getBackgroundColor(item);
		}
		if (role != Qt::DisplayRole) {
			return QVariant();
		}
		return (item->*getter)();
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		return false;
	}

private:
	GETTER getter;
};

#endif
