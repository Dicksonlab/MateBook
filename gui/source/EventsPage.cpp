#include <QtGui>
#include <boost/bind.hpp>
#include "EventsPage.hpp"
#include "../../common/source/Settings.hpp"

EventsPage::EventsPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	QTabWidget* tabWidget = new QTabWidget;

	{	// circling submenu
		QFormLayout* circlingLayout = new QFormLayout;
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_minDistance", "Min Distance", 2, 0, 100, "mm", "Minimum distance between body centroids.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_maxDistance", "Max Distance", 10, 0, 100, "mm", "Maximum distance between body centroids.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_maxAngle", "Max Angle", 60, 0, 180, "°", "Maximum angle towards the other fly.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_minSpeedSelf", "Min Speed Self", 3, 0, 100, "mm/s", "Minimum speed of the circling fly.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_maxSpeedOther", "Max Speed Other", 10, 0, 100, "mm/s", "Maximum speed of the fly being circled.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_minAngleDifference", "Min Angle Difference", 30, 0, 180, "°", "Minimum difference between body orientation and motion direction.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_minSidewaysSpeed", "Min Sideways Speed", 3, 0, 100, "mm/s", "Minimum absolute value of the velocity-component to the left or right of the circling fly.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_medianFilterWidth", "Median Filter Width", 0.2, 0, 100, "s", "Affects all intermediate events.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_persistence", "Persistence", 0.5, 0, 100, "s", "Shorter events are removed.");
		addRowToFormLayout(trackerSettings, circlingLayout, "circling_contribution", "Courtship Contribution", 1, -100, 100, "", "The contribution of this event to weightedCourting.");
		QWidget* dummy = new QWidget;
		dummy->setLayout(circlingLayout);
		tabWidget->addTab(dummy, tr("Circling"));
	}
	
	{	// copulating submenu
		QFormLayout* copulatingLayout = new QFormLayout;
		addRowToFormLayout(trackerSettings, copulatingLayout, "copulating_medianFilterWidth", "Median Filter Width", 0.2, 0, 100, "s", "Affects isOcclusion.");
		addRowToFormLayout(trackerSettings, copulatingLayout, "copulating_persistence", "Persistence", 11, 0, 3600, "s", "Shorter events are removed.");
		addRowToFormLayout(trackerSettings, copulatingLayout, "copulating_contribution", "Courtship Contribution", 0, -100, 100, "", "The contribution of this event to weightedCourting.");
		QWidget* dummy = new QWidget;
		dummy->setLayout(copulatingLayout);
		tabWidget->addTab(dummy, tr("Copulating"));
	}

	{	// following submenu
		QFormLayout* followingLayout = new QFormLayout;
		addRowToFormLayout(trackerSettings, followingLayout, "following_minDistance", "Min Distance", 2, 0, 100, "mm", "Minimum distance between body centroids.");
		addRowToFormLayout(trackerSettings, followingLayout, "following_maxDistance", "Max Distance", 5, 0, 100, "mm", "Maximum distance between body centroids.");
		addRowToFormLayout(trackerSettings, followingLayout, "following_maxAngle", "Max Angle", 60, 0, 180, "°", "Maximum angle towards the other fly.");
		addRowToFormLayout(trackerSettings, followingLayout, "following_minSpeed", "Min Speed", 2, 0, 100, "mm/s", "Minimum speed of either fly.");
		addRowToFormLayout(trackerSettings, followingLayout, "following_maxMovementDirectionDifference", "Max Movement Direction Difference", 90, 0, 180, "°");	//TODO: tooltip
		addRowToFormLayout(trackerSettings, followingLayout, "following_maxChangeOfDistance", "Max Change of Distance", 2, 0, 100, "mm/s");	//TODO: tooltip
		addRowToFormLayout(trackerSettings, followingLayout, "following_medianFilterWidth", "Median Filter Width", 0.2, 0, 100, "s", "Affects all intermediate events.");
		addRowToFormLayout(trackerSettings, followingLayout, "following_persistence", "Persistence", 1, 0, 100, "s", "Shorter events are removed.");
		addRowToFormLayout(trackerSettings, followingLayout, "following_contribution", "Courtship Contribution", 1, -100, 100, "", "The contribution of this event to weightedCourting.");
		QWidget* dummy = new QWidget;
		dummy->setLayout(followingLayout);
		tabWidget->addTab(dummy, tr("Following"));
	}
	
	{	// orienting submenu
		QFormLayout* orientingLayout = new QFormLayout;
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_minDistance", "Min Distance", 3, 0, 100, "mm", "Minimum distance between body centroids.");
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_maxDistance", "Max Distance", 10, 0, 100, "mm", "Maximum distance between body centroids.");
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_maxAngle", "Max Angle", 30, 0, 180, "°", "Maximum angle towards the other fly.");
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_maxSpeedSelf", "Max Speed Self", 1, 0, 100, "mm/s", "Maximum speed of the orienting fly.");
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_maxSpeedOther", "Max Speed Other", 1, 0, 100, "mm/s", "Maximum speed of the other fly.");
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_medianFilterWidth", "Median Filter Width", 0.2, 0, 100, "s", "Affects all intermediate events.");
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_persistence", "Persistence", 1, 0, 100, "s", "Shorter events are removed.");
		addRowToFormLayout(trackerSettings, orientingLayout, "orienting_contribution", "Courtship Contribution", 1, -100, 100, "", "The contribution of this event to weightedCourting.");
		QWidget* dummy = new QWidget;
		dummy->setLayout(orientingLayout);
		tabWidget->addTab(dummy, tr("Orienting"));
	}
	
	{	// rayEllipseOrienting submenu
		QFormLayout* rayEllipseOrientingLayout = new QFormLayout;
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_growthOther", "Growth Other", 1.2, 0, 100, "", "Factor used to resize the other fly to allow for some slack.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_minDistance", "Min Distance", 1.2, 0, 100, "mm", "Minimum distance between body centroids.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_maxDistance", "Max Distance", 10, 0, 100, "mm", "Maximum distance between body centroids.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_maxAngle", "Max Angle", 90, 0, 180, "°", "Maximum angle towards the other fly.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_maxSpeedSelf", "Max Speed Self", 1, 0, 100, "mm/s", "Maximum speed of the orienting fly.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_maxSpeedOther", "Max Speed Other", 10, 0, 100, "mm/s", "Maximum speed of the other fly.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_medianFilterWidth", "Median Filter Width", 0.2, 0, 100, "s", "Affects all intermediate events.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_persistence", "Persistence", 1, 0, 100, "s", "Shorter events are removed.");
		addRowToFormLayout(trackerSettings, rayEllipseOrientingLayout, "rayEllipseOrienting_contribution", "Courtship Contribution", 0, -100, 100, "", "The contribution of this event to weightedCourting.");
		QWidget* dummy = new QWidget;
		dummy->setLayout(rayEllipseOrientingLayout);
		tabWidget->addTab(dummy, tr("Ray-Ellipse Orienting"));
	}
	
	{	// wing extension submenu
		QFormLayout* wingExtensionLayout = new QFormLayout;
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_minAngle", "Min Angle", 30, 0, 180, "°", "Minimum angle between the wing and the major body axis.");
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_directionTolerance", "Direction Tolerance", 15, 0, 180, "°");	//TODO: tooltip
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_tailQuadrantAreaRatio", "Tail Quadrant / Total Area", 0.3, 0, 1, "", "Minimum ratio between the tail quadrant wing area and the total wing area.");
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_angleMedianFilterWidth", "Angle Median Filter Width", 0.2, 0, 100, "s", "Affects the \"Min Angle\".");
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_areaMedianFilterWidth", "Area Median Filter Width", 0.2, 0, 100, "s", "Affects the \"Tail Quadrant / Total Area\".");
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_minBoc", "Min tBoc", 1, -100, 100, "", "Minimum tBoc required to call wingExt during occlusions.");
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_persistence", "Persistence", 0.5, 0, 100, "s", "Shorter events are removed.");
		addRowToFormLayout(trackerSettings, wingExtensionLayout, "wingExtension_contribution", "Courtship Contribution", 1, -100, 100, "", "The contribution of this event to weightedCourting.");
		QWidget* dummy = new QWidget;
		dummy->setLayout(wingExtensionLayout);
		tabWidget->addTab(dummy, tr("Wing Extension"));
	}

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(tabWidget);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
}
