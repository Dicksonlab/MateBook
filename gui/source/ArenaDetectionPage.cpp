#include <QtGui>
#include <boost/bind.hpp>
#include "ArenaDetectionPage.hpp"
#include "../../common/source/Settings.hpp"

ArenaDetectionPage::ArenaDetectionPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	QFormLayout* physicalPropertiesLayout = new QFormLayout;
	arenaDiameter = addRowToFormLayout(trackerSettings, physicalPropertiesLayout, "diameter", "Arena Size", 10, 1, 1000, "mm", "Size of a single arena along its largest dimension.");
	addRowToFormLayout(trackerSettings, physicalPropertiesLayout, "arenaBorderSize", "Arena Border Size", 0.5, 0, 500, "mm", "Size of the arena border caused by parallax.");
	QGroupBox* physicalProperties = new QGroupBox(tr("Physical Properties"));
	physicalProperties->setLayout(physicalPropertiesLayout);

	QVBoxLayout* shapeGroupBoxLayout = new QVBoxLayout;
	QRadioButton* circleRadioButton = new QRadioButton("Circle");
	QRadioButton* rectangleRadioButton = new QRadioButton("Rectangle");
	QRadioButton* ringRadioButton = new QRadioButton("Ring");
	circleRadioButton->setChecked(true);
	shapeButtonGroup = new QButtonGroup;
	shapeButtonGroup->addButton(circleRadioButton, 0);
	shapeButtonGroup->addButton(rectangleRadioButton, 1);
	shapeButtonGroup->addButton(ringRadioButton, 2);
	shapeGroupBoxLayout->addWidget(circleRadioButton);
	shapeGroupBoxLayout->addWidget(rectangleRadioButton);
	shapeGroupBoxLayout->addWidget(ringRadioButton);
	QGroupBox* shape = new QGroupBox(tr("Shape"));
	shape->setLayout(shapeGroupBoxLayout);
	trackerSettings.add<int>("shape", boost::bind(&QButtonGroup::checkedId, shapeButtonGroup), boost::bind(setCheckedId, shapeButtonGroup, _1));

	QVBoxLayout* interiorGroupBoxLayout = new QVBoxLayout;
	QRadioButton* brightInteriorRadioButton = new QRadioButton("Bright");
	QRadioButton* darkInteriorRadioButton = new QRadioButton("Dark");
	QRadioButton* eitherRadioButton = new QRadioButton("Either");
	eitherRadioButton->setChecked(true);
	interiorButtonGroup = new QButtonGroup;
	interiorButtonGroup->addButton(brightInteriorRadioButton, 0);
	interiorButtonGroup->addButton(darkInteriorRadioButton, 1);
	interiorButtonGroup->addButton(eitherRadioButton, 2);
	interiorGroupBoxLayout->addWidget(brightInteriorRadioButton);
	interiorGroupBoxLayout->addWidget(darkInteriorRadioButton);
	interiorGroupBoxLayout->addWidget(eitherRadioButton);
	QGroupBox* interior = new QGroupBox(tr("Interior"));
	interior->setLayout(interiorGroupBoxLayout);
	trackerSettings.add<int>("interior", boost::bind(&QButtonGroup::checkedId, interiorButtonGroup), boost::bind(setCheckedId, interiorButtonGroup, _1));

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(physicalProperties);
	mainLayout->addWidget(shape);
	mainLayout->addWidget(interior);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
}

float ArenaDetectionPage::getArenaDiameter() const
{
	return arenaDiameter->value();
}
