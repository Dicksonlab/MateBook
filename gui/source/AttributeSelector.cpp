#include <QtGui>

#include <cmath>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "AttributeSelector.hpp"
#include "ArenaItem.hpp"
#include "ColorButton.hpp"
#include "makeColorMap.hpp"
#include "../../tracker/source/FrameAttributes.hpp"
#include "../../tracker/source/FlyAttributes.hpp"
#include "../../tracker/source/PairAttributes.hpp"

AttributeSelector::AttributeSelector(QWidget* parent) : QWidget(parent)
{
	QHBoxLayout* layout = new QHBoxLayout;
	attributeComboBox = new QComboBox;
	layout->addWidget(attributeComboBox, 1);
	colorButton = new ColorButton;
	layout->addWidget(colorButton);

	setLayout(layout);

	reset();
}

AttributeSelector::~AttributeSelector()
{
}
/*
QSize AttributeSelector::minimumSizeHint() const
{
	return QSize(160, 100);
}

QSize AttributeSelector::sizeHint() const
{
	return QSize(340, 100);
}
*/

std::string AttributeSelector::getString() const
{
	QVariant itemData = attributeComboBox->itemData(attributeComboBox->currentIndex());
	if (!itemData.isValid() || !itemData.canConvert<QString>()) {
		std::cerr << "cannot create attribute string in AttributeSelector::getString(): no attribute selected" << std::endl;
		return std::string();
	}

	QColor color = colorButton->getColor();

	std::string description =
		//attributeComboBox->currentText().toStdString() + delimiter +
		itemData.toString().toStdString() + delimiter +
		stringify(color.red()) + delimiter +
		stringify(color.green()) + delimiter +
		stringify(color.blue())
	;

	return description;
}

void AttributeSelector::fromString(const std::string& description)
{
	const int numberOfTokens = 5;	// the number of tokens that have been serialized

	if (description == "") {	// deselect attribute
		attributeComboBox->setCurrentIndex(0);
		colorButton->setColor(Qt::black);
		return;
	}

	const std::vector<std::string> splitDescription = split(description, delimiter);

	if (splitDescription.size() != numberOfTokens) {
		std::cerr << "cannot restore values in AttributeSelector::fromString(): expected " << numberOfTokens << " tokens, but got " << splitDescription.size() << std::endl;
		return;
	}

	QString attributeName = QString::fromStdString(splitDescription[0]);
	QString attributeKind = QString::fromStdString(splitDescription[1]);

	int red, green, blue;

	try {
		red = unstringify<int>(splitDescription[2]);
		green = unstringify<int>(splitDescription[3]);
		blue = unstringify<int>(splitDescription[4]);
	} catch (std::runtime_error& e) {
		std::cerr << "cannot restore values in AttributeSelector::fromString(): " << e.what() << std::endl;
		return;
	}

	for (int attributeNumber = 0; attributeNumber != attributeComboBox->count(); ++attributeNumber) {
		if (//attributeComboBox->itemText(attributeNumber) == attributeName &&
			attributeComboBox->itemData(attributeNumber).canConvert<QString>() &&
			attributeComboBox->itemData(attributeNumber).toString() == attributeName+delimiter+attributeKind
		) {
			attributeComboBox->setCurrentIndex(attributeNumber);
			break;
		}
	}

	colorButton->setColor(QColor(red, green, blue));
}

void AttributeSelector::selectAttribute(const QString& name, const QString& kind)
{
	for (int attributeNumber = 0; attributeNumber != attributeComboBox->count(); ++attributeNumber) {
		if (//attributeComboBox->itemText(attributeNumber) == name &&
			attributeComboBox->itemData(attributeNumber).canConvert<QString>() &&
			attributeComboBox->itemData(attributeNumber).toString() == name+delimiter+kind
		) {
			attributeComboBox->setCurrentIndex(attributeNumber);
			return;
		}
	}

	std::cerr << "cannot select " << kind.toStdString() << " attribute " << name.toStdString() << ": it is not in the list." << std::endl;
}

void AttributeSelector::setColor(const QColor& color)
{
	colorButton->setColor(color);
}

void AttributeSelector::reset()	//TODO: add only MyBool attributes to the combo box (the tracker doesn't know how to deal with the others)
{
	attributeComboBox->clear();
	attributeComboBox->addItem("<attribute>", QVariant());
	attributeComboBox->insertSeparator(attributeComboBox->count());
	FrameAttributes frameAttributes = FrameAttributes();
	{
		std::vector<std::string> attributeNames = frameAttributes.getNames<MyBool>();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
      std::string shortname = frameAttributes.get(*iter).getShortName();
      if(shortname.empty())  continue;
			attributeComboBox->addItem(QString::fromStdString(shortname),
            QString::fromStdString(*iter+delimiter+"frame"));
		}
	}
	attributeComboBox->insertSeparator(attributeComboBox->count());
	FlyAttributes flyAttributes = FlyAttributes();
	{
		std::vector<std::string> attributeNames = flyAttributes.getNames<MyBool>();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
      std::string shortname = flyAttributes.get(*iter).getShortName();
      if(shortname.empty())  continue;
			attributeComboBox->addItem(QString::fromStdString(shortname),
            QString::fromStdString(*iter+delimiter+"fly"));
		}
	}
	attributeComboBox->insertSeparator(attributeComboBox->count());
	PairAttributes pairAttributes = PairAttributes();
	{
		std::vector<std::string> attributeNames = PairAttributes().getNames<MyBool>();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
      std::string shortname = pairAttributes.get(*iter).getShortName();
      if(shortname.empty())  continue;
			attributeComboBox->addItem(QString::fromStdString(shortname),
            QString::fromStdString(*iter+delimiter+"pair"));
		}
	}
}
