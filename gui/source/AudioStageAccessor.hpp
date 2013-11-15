#ifndef AudioStageAccessor_hpp
#define AudioStageAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  AudioStageAccessor
  * @brief  to get the current audio processing stage of a Item
  */
template<class ITEMBASE>
class AudioStageAccessor : public Accessor<ITEMBASE> {
public:
	AudioStageAccessor() : Accessor<ITEMBASE>()
	{
	}

	Qt::ItemFlags flags(const ITEMBASE* item) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	QVariant data(const ITEMBASE* item, int role) const
	{
		if (role == Qt::DisplayRole) {
			Item::AudioStage currentAudioStage = item->getCurrentAudioStage();
			switch (currentAudioStage) {
				case Item::AudioRecording: {
					return "Recording";
				}
				case Item::PulseDetection: {
					return "Pulse Detection";
				}
				case Item::ModifyingFile:{
					return "Modifying File";
				}
				case Item::StatisticalAudioAnalysis: {
					return "Statistical Analysis";
				}
				default: {
					return "unknown stage";
				}
			}
		} else if (role == Qt::EditRole || role == Qt::ToolTipRole) {
			Item::AudioStage currentAudioStage = item->getCurrentAudioStage();
			switch (currentAudioStage) {
				case Item::AudioRecording: {
					return "Recording";
				}
				case Item::PulseDetection: {
					return "Pulse Detection";
				}
			    case Item::ModifyingFile:{
					return "Modifying File";
				}
				case Item::StatisticalAudioAnalysis: {
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
