#include <QtGui>
#include <boost/bind.hpp>
#include "ConfigPage.hpp"
#include "../../common/source/Settings.hpp"

ConfigPage::ConfigPage(Settings& trackerSettings, QWidget* parent) : QWidget(parent)
{
}

void ConfigPage::readGuiSettings(const QSettings& settings)
{
	// empty implementations so that derived classes are not forced to implement it
}

void ConfigPage::writeGuiSettings(QSettings& settings) const
{
	// empty implementations so that derived classes are not forced to implement it
}

QDoubleSpinBox* addRowToFormLayout(Settings& settings, QFormLayout* layout, const QString& key, const QString& labelText, double defaultValue, double minValue, double maxValue, const QString& unit, const QString& labelToolTip, const QString& fieldToolTip)
{
	QDoubleSpinBox* spinBox = new QDoubleSpinBox;
	spinBox->setRange(minValue, maxValue);
	spinBox->setValue(defaultValue);
	// make the spinbox buttons change the value by roughly 1/10th of the default value
	double singleStep = std::pow(10.0, static_cast<double>(static_cast<int>(std::log10(std::abs(defaultValue))) - 1));
	spinBox->setSingleStep(defaultValue == 0 ? 1.0 : singleStep);
	spinBox->setSuffix(QString(" ") + unit);
	layout->addRow(labelText + QString(": "), spinBox);
	layout->itemAt(layout->count() - 2)->widget()->setToolTip(labelToolTip);
	layout->itemAt(layout->count() - 1)->widget()->setToolTip(key);	//TODO: save key elsewhere and set this to fieldToolTip (or for instance the allowed range by default)
	settings.add<double>(key.toStdString(), boost::bind(&QDoubleSpinBox::value, spinBox), boost::bind(&QDoubleSpinBox::setValue, spinBox, _1));
	return spinBox;
}

void setCheckedId(QButtonGroup* buttonGroup, int id)
{
	if (QAbstractButton* button = buttonGroup->button(id)) {
		button->setChecked(true);
	}
}
