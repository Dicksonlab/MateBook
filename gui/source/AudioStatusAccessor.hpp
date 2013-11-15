#ifndef AudioStatusAccessor_hpp
#define AudioStatusAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  AudioStatusAccessor
  * @brief  to get the current audio processing status of a Item
  */
template<class ITEMBASE>
class AudioStatusAccessor : public Accessor<ITEMBASE> {
public:
	AudioStatusAccessor() : Accessor<ITEMBASE>()
	{
	}

	Qt::ItemFlags flags(const ITEMBASE* item) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	QVariant data(const ITEMBASE* item, int role) const
	{
		if (role == Qt::DisplayRole) {
			Item::Status currentStatus = item->getCurrentAudioStatus();
			switch (currentStatus) {
				case Item::Queued: {
					return "Queued";
				}
				case Item::Started: {
					return "Started";
				}
				case Item::Finished: {
					return "Finished";
				}
				case Item::Failed: {
					return "Failed";
				}
				default: {
					return "unknown status";
				}
			}
		} else if (role == Qt::EditRole || role == Qt::ToolTipRole) {
			Item::Status currentStatus = item->getCurrentAudioStatus();
			switch (currentStatus) {
				case Item::Queued: {
					return "Queued";
				}
				case Item::Started: {
					return "Started";
				}
				case Item::Finished: {
					return "Finished";
				}
				case Item::Failed: {
					return "Failed";
				}
				default: {
					return "unknown status";
				}
			}
		} else if (role == Qt::BackgroundRole) {
			return this->getBackgroundColor(item);
		} else {
			return QVariant();
		}
	}

	bool setData(ITEMBASE* item, const QVariant& value, int role) const
	{
		return false;
	}

private:
};

#endif
