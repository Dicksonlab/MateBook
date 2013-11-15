#ifndef TimeAccessor_hpp
#define TimeAccessor_hpp

#include "Accessor.hpp"

#include <QStringList>

/**
  * @class  TimeAccessor
  * 
  */
template<class GETTER, class SETTER>
class TimeAccessor : public Accessor<Item> {
public:
	TimeAccessor(GETTER getter, SETTER setter) : Accessor<Item>(),
		getter(getter),
		setter(setter)
	{
	}

	Qt::ItemFlags flags(const Item* item) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	}

	QVariant data(const Item* item, int role) const
	{
		if (role == Qt::BackgroundRole) {
			return this->getBackgroundColor(item);
		}
		if (role != Qt::DisplayRole && role != Qt::EditRole) {
			return QVariant();
		}
		unsigned int time = (item->*getter)();
		return intSecondsToTimeString(time);
	}

	bool setData(Item* item, const QVariant& value, int role) const
	{
		if(role == Qt::EditRole){
				(item->*setter)(stringToIntSeconds(value.toString()));
				return true;
		}
		return false;
	}

private:
	GETTER getter;
	SETTER setter;

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

	unsigned int stringToIntSeconds(const QString& s) const
	{
		QStringList temp = s.split(":");

		if(temp.size() == 3){
			unsigned int tempTime = temp[2].toInt();
			tempTime += temp[1].toInt()*60;
			tempTime += temp[0].toInt()*3600;

			return tempTime;
		}
		return 0;
	}
};

#endif
