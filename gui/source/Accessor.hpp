#ifndef Accessor_hpp
#define Accessor_hpp

#include <QString>
#include <QVariant>
#include <QAbstractItemModel>

#include "Item.hpp"

/**
  * @class  Accessor
  * @brief  translates C++ types to those provided by a QAbstractItemModel
  */
template<class ITEMBASE>
class Accessor {
public:
	virtual Qt::ItemFlags flags(const ITEMBASE* item) const = 0;
	virtual QVariant data(const ITEMBASE* item, int role) const = 0;
	virtual bool setData(ITEMBASE* item, const QVariant& value, int role) const = 0;

	// rows of videos are colored according to their stage 
	// and status (using Qt::BackgroundRole)
	QVariant getBackgroundColor(const ITEMBASE* item) const
	{
		Item::Status currentStatus = item->getCurrentVideoStatus() > item->getCurrentAudioStatus()? item->getCurrentVideoStatus() : item->getCurrentAudioStatus();

		switch (currentStatus) {
			case Item::Queued: {
				return QVariant(QColor(238, 216, 174)); //orange
			}
			case Item::Started: {
				return QVariant(QColor(255, 255, 224)); //yellow
			}
			case Item::Finished: {
				return QVariant(QColor(209, 238, 238)); //green
			}
		    case Item::Failed: {
				return QVariant(QColor(188, 143, 143)); //red
			}
			default: {
				return QVariant(QColor(Qt::white));
			}
		}
	}
};

#endif
