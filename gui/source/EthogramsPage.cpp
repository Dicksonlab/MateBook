#include <QtGui>
#include <boost/bind.hpp>
#include "EthogramsPage.hpp"
#include "VerticalWidgetList.hpp"
#include "AttributeSelector.hpp"
#include "../../common/source/Settings.hpp"
#include "../../common/source/debug.hpp"
#include "../../common/source/stringUtilities.hpp"

// helper function for the VerticalWidgetList
QWidget* createEthogramAttributeSelector()
{
	return new AttributeSelector;
}

EthogramsPage::EthogramsPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	QVBoxLayout* attributesLayout = new QVBoxLayout;
	ethogramAttributes = new VerticalWidgetList(createEthogramAttributeSelector);
	ethogramAttributes->setContentAlignment(Qt::AlignTop);
	ethogramAttributes->setWidgetCount(5);

	// default attributes to be shown in ethograms
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(0))->selectAttribute("copulating", "fly");
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(0))->setColor(QColor(255, 255, 0));
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(1))->selectAttribute("circling", "pair");
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(1))->setColor(QColor(200, 81, 255));
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(2))->selectAttribute("wingExtTowards", "pair");
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(2))->setColor(QColor(255, 0, 0));
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(3))->selectAttribute("courting", "fly");
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(3))->setColor(QColor(0, 255, 0));
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(4))->selectAttribute("orienting", "pair");
	assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(4))->setColor(QColor(81, 124, 255));

	attributesLayout->addWidget(ethogramAttributes);
	QGroupBox* attributes = new QGroupBox(tr("Attributes"));
	attributes->setLayout(attributesLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(attributes);
	mainLayout->addStretch(1);
	setLayout(mainLayout);

	trackerSettings.add<std::string>("ethogramSpecification", boost::bind(&EthogramsPage::getString, this), boost::bind(&EthogramsPage::fromString, this, _1));
}

std::string EthogramsPage::getString() const
{
	const char delimiter = '|';

	std::string description;
	for (size_t ethogramNumber = 0; ethogramNumber != ethogramAttributes->count(); ++ethogramNumber) {
		const AttributeSelector* attributeSelector = assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(ethogramNumber));
		description += attributeSelector->getString();
		if (ethogramNumber + 1 != ethogramAttributes->count()) {	//TODO: is this necessary or does the split() function anyway take care of not returning empty trailing tokens?
			description += delimiter;
		}
	}

	return description;
}

void EthogramsPage::fromString(const std::string& description)
{
	const char delimiter = '|';

	const std::vector<std::string> splitDescription = split(description, delimiter);
	ethogramAttributes->setWidgetCount(splitDescription.size());

	for (size_t ethogramNumber = 0; ethogramNumber != ethogramAttributes->count(); ++ethogramNumber) {
		AttributeSelector* attributeSelector = assert_cast<AttributeSelector*>(ethogramAttributes->getWidget(ethogramNumber));
		attributeSelector->fromString(splitDescription[ethogramNumber]);
	}
}
