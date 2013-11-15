#ifndef VideoStageAccessor_hpp
#define VideoStageAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  VideoStageAccessor
  * @brief  to get the current video processing stage of a Item
  */
template<class ITEMBASE>
class VideoStageAccessor : public Accessor<ITEMBASE> {
public:
	VideoStageAccessor() : Accessor<ITEMBASE>()
	{
	}

	Qt::ItemFlags flags(const ITEMBASE* item) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	QVariant data(const ITEMBASE* item, int role) const
	{
		if (role == Qt::DisplayRole) {
			Item::VideoStage currentStage = item->getCurrentVideoStage();
			switch (currentStage) {
				case Item::VideoRecording: {
					return "Recording";
				}
				case Item::ArenaDetection: {
					return "Arena Detection";
				}
				case Item::FlyTracking: {
					return "Fly Tracking";
				}
				case Item::StatisticalVideoAnalysis: {
					return "Statistical Analysis";
				}
				default: {
					return "unknown stage";
				}
			}
		} else if (role == Qt::EditRole || role == Qt::ToolTipRole) {
			Item::VideoStage currentStage = item->getCurrentVideoStage();
			switch (currentStage) {
				case Item::VideoRecording: {
					return "Recording";
				}
				case Item::ArenaDetection: {
					return "Arena Detection";
				}
				case Item::FlyTracking: {
					return "Fly Tracking";
				}
				case Item::StatisticalVideoAnalysis: {
					return "Statistical Analysis";
				}
				default: {
					return "unknown stage";
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
