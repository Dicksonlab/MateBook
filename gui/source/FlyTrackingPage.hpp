#ifndef FlyTrackingPage_hpp
#define FlyTrackingPage_hpp

#include <QWidget>
#include "ConfigPage.hpp"

class Settings;

class FlyTrackingPage : public ConfigPage {
public:
	FlyTrackingPage(Settings& trackerSettings, QWidget* parent = 0);

private:
	QCheckBox* saveContours;
	QCheckBox* saveHistograms;
	QCheckBox* gradientCorrection;
	QCheckBox* discardMissegmentations;
	QCheckBox* fullyMergeMissegmentations;
	QCheckBox* splitBodies;
	QCheckBox* splitWings;
	QCheckBox* useManualOcclusionSolution;
};

#endif
