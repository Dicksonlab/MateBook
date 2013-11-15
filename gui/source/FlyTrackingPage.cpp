#include <QtGui>
#include <boost/bind.hpp>
#include "FlyTrackingPage.hpp"
#include "../../common/source/Settings.hpp"

FlyTrackingPage::FlyTrackingPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	QFormLayout* hintsLayout = new QFormLayout;
	{
		QSpinBox* spinBox = new QSpinBox;
		spinBox->setRange(1, 2);
		spinBox->setValue(2);
		hintsLayout->addRow("Flies per Arena" + QString(": "), spinBox);
		hintsLayout->itemAt(hintsLayout->count() - 2)->widget()->setToolTip("Improves segmentation and is currently required for postprocessing.");
		hintsLayout->itemAt(hintsLayout->count() - 1)->widget()->setToolTip("hints_flyCount");	//TODO: save key elsewhere and set this to fieldToolTip (or for instance the allowed range by default)
		trackerSettings.add<int>("hints_flyCount", boost::bind(&QSpinBox::value, spinBox), boost::bind(&QSpinBox::setValue, spinBox, _1));
	}

	QGroupBox* hints = new QGroupBox(tr("Hints"));
	hints->setLayout(hintsLayout);

	saveContours = new QCheckBox(this);
	saveContours->setText("Save Contours");
	saveContours->setToolTip("Fly contours require a lot of storage space, but are useful to judge the quality of segmentation.");
	trackerSettings.add<bool>("contours", boost::bind(&QCheckBox::isChecked, saveContours), boost::bind(&QCheckBox::setChecked, saveContours, _1));

	saveHistograms = new QCheckBox(this);
	saveHistograms->setText("Save Histograms");
	saveHistograms->setToolTip("Histograms require a lot of storage space, but are useful to debug the segmentation threshold selection method.");
	trackerSettings.add<bool>("histograms", boost::bind(&QCheckBox::isChecked, saveHistograms), boost::bind(&QCheckBox::setChecked, saveHistograms, _1));

	gradientCorrection = new QCheckBox(this);
	gradientCorrection->setText("Gradient Correction");
	gradientCorrection->setToolTip("Improves body segmentation by expanding the body area to the surrounding edge.");
	trackerSettings.add<bool>("segmentation_gradientCorrection", boost::bind(&QCheckBox::isChecked, gradientCorrection), boost::bind(&QCheckBox::setChecked, gradientCorrection, _1));

	discardMissegmentations = new QCheckBox(this);
	discardMissegmentations->setText("Discard Missegmentations");
	discardMissegmentations->setToolTip("Discards missegmented frames when calculating s-Scores.");
	trackerSettings.add<bool>("discardMissegmentations", boost::bind(&QCheckBox::isChecked, discardMissegmentations), boost::bind(&QCheckBox::setChecked, discardMissegmentations, _1));

	fullyMergeMissegmentations = new QCheckBox(this);
	fullyMergeMissegmentations->setText("Fully Merge Missegmentations");
	fullyMergeMissegmentations->setToolTip("Merges missegmented bodies within the same wing. May improve wingExt recovery but also generate more occlusions.");
	trackerSettings.add<bool>("fullyMergeMissegmentations", boost::bind(&QCheckBox::isChecked, fullyMergeMissegmentations), boost::bind(&QCheckBox::setChecked, fullyMergeMissegmentations, _1));

	splitBodies = new QCheckBox(this);
	splitBodies->setText("Split Bodies");
	splitBodies->setToolTip("Attempts to split bodies during occlusions.");
	splitBodies->setChecked(true);
	trackerSettings.add<bool>("splitBodies", boost::bind(&QCheckBox::isChecked, splitBodies), boost::bind(&QCheckBox::setChecked, splitBodies, _1));

	splitWings = new QCheckBox(this);
	splitWings->setText("Split Wings");
	splitWings->setToolTip("Attempts to split wing areas to recover wingExt events.");
	splitWings->setChecked(true);
	trackerSettings.add<bool>("splitWings", boost::bind(&QCheckBox::isChecked, splitWings), boost::bind(&QCheckBox::setChecked, splitWings, _1));

	QVBoxLayout* forDebuggingLayout = new QVBoxLayout;
	forDebuggingLayout->addWidget(saveContours);
	forDebuggingLayout->addWidget(saveHistograms);
	forDebuggingLayout->addWidget(gradientCorrection);
	forDebuggingLayout->addWidget(discardMissegmentations);
	forDebuggingLayout->addWidget(fullyMergeMissegmentations);
	forDebuggingLayout->addWidget(splitBodies);
	forDebuggingLayout->addWidget(splitWings);
	QGroupBox* forDebugging = new QGroupBox(tr("For Debugging"));
	forDebugging->setLayout(forDebuggingLayout);

	QFormLayout* segmentationLayout = new QFormLayout;
	addRowToFormLayout(trackerSettings, segmentationLayout, "segmentation_thresholdOffset", "Threshold Offset", 0, -100, 100, "", "Adjusts the automatically determined threshold.");
	addRowToFormLayout(trackerSettings, segmentationLayout, "segmentation_minFlyBodySize", "Minimum Fly Body Size", 0.5, 0, 10000, "mm²", "Smaller areas are considered a missegmentation.");
	addRowToFormLayout(trackerSettings, segmentationLayout, "segmentation_maxFlyBodySize", "Maximum Fly Body Size", 2, 0, 10000, "mm²", "Larger areas are considered a missegmentation.");
	QGroupBox* segmentation = new QGroupBox(tr("Segmentation"));
	segmentation->setLayout(segmentationLayout);

	QFormLayout* occlusionsLayout = new QFormLayout;
	addRowToFormLayout(trackerSettings, occlusionsLayout, "occlusions_sSize", "sSize Linear Weight", 1, -100, 100, "", "Effect of fly body sizes on occlusion resolution.");
	addRowToFormLayout(trackerSettings, occlusionsLayout, "occlusions_tPos", "tPos Logistic Regression Coefficient", 6.60, -100, 100, "", "Effect of fly positions on occlusion resolution.");
	addRowToFormLayout(trackerSettings, occlusionsLayout, "occlusions_tBoc", "tBoc Logistic Regression Coefficient", 5.32, -100, 100, "", "Effect of contour tracking on occlusion resolution.");
	QGroupBox* occlusions = new QGroupBox(tr("Occlusions"));
	occlusions->setLayout(occlusionsLayout);

	QFormLayout* headingLayout = new QFormLayout;
	addRowToFormLayout(trackerSettings, headingLayout, "heading_sMotion", "sMotion Weight", 0, -100, 100, "", "Effect of fly walking directions on heading resolution.");
	addRowToFormLayout(trackerSettings, headingLayout, "heading_sWings", "sWings Weight", 0, -100, 100, "", "Effect of wing positions on heading resolution.");
	addRowToFormLayout(trackerSettings, headingLayout, "heading_sMaxMotionWings", "sMaxMotionWings Weight", 1, -100, 100, "", "Effect of the more confident of the above on heading resolution.");
	addRowToFormLayout(trackerSettings, headingLayout, "heading_sColor", "sColor Weight", 0, -100, 100, "", "Effect of the body color distribution on heading resolution.");
	addRowToFormLayout(trackerSettings, headingLayout, "heading_tBefore", "tBefore Weight", 6, -100, 100, "", "Effect of the orientation change on heading resolution.");
	QGroupBox* heading = new QGroupBox(tr("Heading"));
	heading->setLayout(headingLayout);

	QVBoxLayout* annotationsLayout = new QVBoxLayout;
	useManualOcclusionSolution = new QCheckBox(this);
	useManualOcclusionSolution->setText("Use Manual Occlusion Solution");
	useManualOcclusionSolution->setToolTip("Resolves occlusions in the same way as in the annotation file.");
	useManualOcclusionSolution->setChecked(true);
	trackerSettings.add<bool>("annotation", boost::bind(&QCheckBox::isChecked, useManualOcclusionSolution), boost::bind(&QCheckBox::setChecked, useManualOcclusionSolution, _1));

	annotationsLayout->addWidget(useManualOcclusionSolution);
	QGroupBox* annotations = new QGroupBox(tr("Annotations"));
	annotations->setLayout(annotationsLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(hints);
	mainLayout->addWidget(forDebugging);
	mainLayout->addWidget(segmentation);
	mainLayout->addWidget(occlusions);
	mainLayout->addWidget(heading);
	mainLayout->addWidget(annotations);
	mainLayout->addStretch(1);

	QScrollArea* buttonsScrollArea = new QScrollArea;
	buttonsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	buttonsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	QWidget* dummyScrollAreaWidget = new QWidget;
	dummyScrollAreaWidget->setLayout(mainLayout);
	buttonsScrollArea->setWidget(dummyScrollAreaWidget);
	QVBoxLayout* dummyLayout = new QVBoxLayout;
	dummyLayout->addWidget(buttonsScrollArea);

	setLayout(dummyLayout);
}
