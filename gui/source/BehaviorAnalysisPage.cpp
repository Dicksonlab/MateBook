#include <QtGui>
#include <boost/bind.hpp>
#include "BehaviorAnalysisPage.hpp"
#include "VerticalWidgetList.hpp"
#include "Heatmapper.hpp"
#include "../../common/source/Settings.hpp"
#include "../../common/source/debug.hpp"

// helper function for the VerticalWidgetList
QWidget* createHeatmapper()
{
	return new Heatmapper;
}

BehaviorAnalysisPage::BehaviorAnalysisPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	QFormLayout* binningLayout = new QFormLayout;
	addRowToFormLayout(trackerSettings, binningLayout, "binSize", "Bin Size", 60, 1, 3600, "s", "Time duration summarized by a single bin.");
	{
		QSpinBox* spinBox = new QSpinBox;
		spinBox->setRange(0, 1000);
		spinBox->setValue(10);
		binningLayout->addRow("Bin Count" + QString(": "), spinBox);
		binningLayout->itemAt(binningLayout->count() - 2)->widget()->setToolTip("Number of bins written to behavior reports.");
		binningLayout->itemAt(binningLayout->count() - 1)->widget()->setToolTip("binCount");	//TODO: save key elsewhere and set this to fieldToolTip (or for instance the allowed range by default)
		trackerSettings.add<int>("binCount", boost::bind(&QSpinBox::value, spinBox), boost::bind(&QSpinBox::setValue, spinBox, _1));
	}
	QGroupBox* binning = new QGroupBox(tr("Binning"));
	binning->setLayout(binningLayout);

	QVBoxLayout* heatmappersLayout = new QVBoxLayout;
	heatmappers = new VerticalWidgetList(createHeatmapper);
	heatmappers->setWidgetCount(1);
	heatmappersLayout->addWidget(heatmappers);
	QGroupBox* heatmaps = new QGroupBox(tr("Heatmaps"));
	heatmaps->setLayout(heatmappersLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(binning);
	mainLayout->addWidget(heatmaps);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
}

std::vector<QImage> BehaviorAnalysisPage::getHeatmaps(const std::vector<ArenaItem*>& arenaItems) const
{
	std::vector<QImage> heatmaps;
	for (int i = 0; i < heatmappers->count(); ++i) {
		Heatmapper* heatmapper = assert_cast<Heatmapper*>(heatmappers->getWidget(i));
		heatmaps.push_back(heatmapper->drawHeatmap(arenaItems));
	}
	return heatmaps;
}
