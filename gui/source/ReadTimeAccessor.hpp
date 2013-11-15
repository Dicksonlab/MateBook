#ifndef ReadTimeAccessor_hpp
#define ReadTimeAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  ReadTimeAccessor
  * @brief  reads time as usigned int in seconds and returns a string
  *
  */
template<class T, class GETTER>
class ReadTimeAccessor : public Accessor<Item> {
public:
	ReadTimeAccessor(GETTER getter) : Accessor<Item>(),
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
		unsigned int time = (item->*getter)();
		return intSecondsToTimeString(time);
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		return false;
	}

protected:
	GETTER getter;

	QString intSecondsToTimeString(const unsigned int t) const
	{
		unsigned int tempTime = t;
		unsigned int hours = tempTime / 3600;
		tempTime -= hours * 3600;
		unsigned int minutes = tempTime / 60;
		tempTime -= minutes * 60;
		unsigned int seconds = tempTime;

		return QString::number(hours).rightJustified(2, '0') + ":" + QString::number(minutes).rightJustified(2, '0') + ":" + 
			   QString::number(seconds).rightJustified(2, '0');
	}
};

#endif
