#include <QtGui>
#include <boost/bind.hpp>
#include "DisplayPage.hpp"
#include "../../common/source/Settings.hpp"

DisplayPage::DisplayPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	QFormLayout* colorsLayout = new QFormLayout;
	fly0ColorButton = new ColorButton(QColor::fromHsvF(200.0 / 360.0, 0.5, 1));
	colorsLayout->addRow(QString("1st Fly: "), fly0ColorButton);
	colorsLayout->itemAt(colorsLayout->count() - 2)->widget()->setToolTip(QString("The color used in the user interface to visualize data for the first fly."));
	colorsLayout->itemAt(colorsLayout->count() - 1)->widget()->setToolTip(QString("Click to select color."));
	fly1ColorButton = new ColorButton(QColor::fromHsvF(315.0 / 360.0, 0.5, 1));
	colorsLayout->addRow(QString("2nd Fly: "), fly1ColorButton);
	colorsLayout->itemAt(colorsLayout->count() - 2)->widget()->setToolTip(QString("The color used in the user interface to visualize data for the second fly."));
	colorsLayout->itemAt(colorsLayout->count() - 1)->widget()->setToolTip(QString("Click to select color."));
	QGroupBox* colors = new QGroupBox(tr("Fly Colors"));
	colors->setLayout(colorsLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(colors);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
}
