#ifndef ImageAccessor_hpp
#define ImageAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  ImageAccessor
  * @brief  to access images the Item can return
  *
  * GETTER is an Item member function returning a QIcon, QPixmap, QImage or QColor (see the QStyledItemDelegate docs)
  */
template<class T, class GETTER>
class ImageAccessor : public Accessor<Item> {
public:
	ImageAccessor(GETTER getter) : Accessor<Item>(),
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
		if (role == Qt::DecorationRole) {
			return (item->*getter)();
		}
		if (role == Qt::SizeHintRole) {
			return (item->*getter)().size();
		}
		return QVariant();
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		return false;
	}

private:
	GETTER getter;
};

#endif
