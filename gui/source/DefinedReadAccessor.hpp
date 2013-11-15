#ifndef DefinedReadAccessor_hpp
#define DefinedReadAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  DefinedReadAccessor
  * @brief  to read Item properties that have a getter function and which can be undefined
  *
  * GETTER is a Item member function returning a value convertible to a QVariant
  */
template<class T, class DEFINED, class GETTER>
class DefinedReadAccessor : public Accessor<Item> {
public:
	DefinedReadAccessor(DEFINED defined, GETTER getter) : Accessor<Item>(),
		defined(defined),
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
		if (role == Qt::DisplayRole && (item->*defined)()) {
			return (item->*getter)();
		}
		return QVariant();
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		return false;
	}

private:
	DEFINED defined;
	GETTER getter;
};

#endif
