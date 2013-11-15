#ifndef BehaviorAnalysisPage_hpp
#define BehaviorAnalysisPage_hpp

#include <QWidget>
#include "ConfigPage.hpp"

class Settings;
class ArenaItem;
class VerticalWidgetList;

class BehaviorAnalysisPage : public ConfigPage {
public:
	BehaviorAnalysisPage(Settings& trackerSettings, QWidget* parent = 0);

	std::vector<QImage> getHeatmaps(const std::vector<ArenaItem*>& arenaItems) const;

private:
	VerticalWidgetList* heatmappers;
};

#endif
