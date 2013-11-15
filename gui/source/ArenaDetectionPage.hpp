#ifndef ArenaDetectionPage_hpp
#define ArenaDetectionPage_hpp

#include <QWidget>
#include "ConfigPage.hpp"

class Settings;

class ArenaDetectionPage : public ConfigPage {
public:
	ArenaDetectionPage(Settings& trackerSettings, QWidget* parent = 0);

	float getArenaDiameter() const;

private:
	QDoubleSpinBox* arenaDiameter;
	QButtonGroup* shapeButtonGroup;
	QButtonGroup* interiorButtonGroup;
};

#endif
