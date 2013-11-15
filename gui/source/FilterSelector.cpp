#include <QtGui>

#include <cmath>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "FilterSelector.hpp"
#include "ArenaItem.hpp"
#include "ColorButton.hpp"
#include "makeColorMap.hpp"
#include "../../tracker/source/FrameAttributes.hpp"
#include "../../tracker/source/FlyAttributes.hpp"
#include "../../tracker/source/PairAttributes.hpp"

FilterSelector::FilterSelector(QWidget* parent) : QWidget(parent)
{
	QVBoxLayout* verticalLayout = new QVBoxLayout;

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	attributeComboBox = new QComboBox;
	horizontalLayout->addWidget(attributeComboBox, 3);
	dimensionComboBox = new QComboBox;
	horizontalLayout->addWidget(dimensionComboBox, 1);
	activeFlyComboBox = new QComboBox;
	horizontalLayout->addWidget(activeFlyComboBox, 1);
	passiveFlyComboBox = new QComboBox;
	horizontalLayout->addWidget(passiveFlyComboBox, 1);
	operatorComboBox = new QComboBox;
	horizontalLayout->addWidget(operatorComboBox, 1);
	intSpinBox = new QSpinBox;
	horizontalLayout->addWidget(intSpinBox, 1);
	realSpinBox = new QDoubleSpinBox;
	horizontalLayout->addWidget(realSpinBox, 1);
	unitComboBox = new QComboBox;
	horizontalLayout->addWidget(unitComboBox, 1);

	verticalLayout->addLayout(horizontalLayout);

	setLayout(verticalLayout);

	reset();

	connect(attributeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(attributeChanged()));
//	connect(dimensionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(dimensionChanged(int)));
}

FilterSelector::~FilterSelector()
{
}

QSize FilterSelector::minimumSizeHint() const
{
	return QSize(160, 100);
}

QSize FilterSelector::sizeHint() const
{
	return QSize(340, 100);
}

boost::shared_ptr<Filter> FilterSelector::getFilter() const
{
	QVariant itemData = attributeComboBox->itemData(attributeComboBox->currentIndex());
	if (itemData.isValid() && itemData.canConvert(QVariant::StringList)) {
		QStringList attributeInfo = itemData.toStringList();
		assert(attributeInfo.size() == 2);
		QString attributeName = attributeComboBox->currentText();
		QString attributeKind = attributeInfo[0];
		QString attributeType = attributeInfo[1];
		
		unsigned int dimension = 0;
		if (attributeType == "Vec<float,2>") {
			QVariant dimensionData = dimensionComboBox->itemData(dimensionComboBox->currentIndex());
			if (dimensionData.isValid() && dimensionData.canConvert(QVariant::UInt)) {
				dimension = dimensionData.toUInt();
			}
		}

		unsigned int activeFly = 0;
		if (attributeKind == "fly" || attributeKind == "pair") {
			QVariant activeData = activeFlyComboBox->itemData(activeFlyComboBox->currentIndex());
			if (activeData.isValid() && activeData.canConvert(QVariant::UInt)) {
				activeFly = activeData.toUInt();
			} else {
				std::cerr << "WARNING: returning NULL Filter in FilterSelector::getFilter()" << std::endl;
				return boost::shared_ptr<Filter>();
			}
		}

		unsigned int passiveFly = 0;
		if (attributeKind == "pair") {
			QVariant passiveData = passiveFlyComboBox->itemData(passiveFlyComboBox->currentIndex());
			if (passiveData.isValid() && passiveData.canConvert(QVariant::UInt)) {
				passiveFly = passiveData.toUInt();
			} else {
				std::cerr << "WARNING: returning NULL Filter in FilterSelector::getFilter()" << std::endl;
				return boost::shared_ptr<Filter>();
			}
		}

		if (operatorComboBox->currentText() == "=") {
			if (attributeType == "Vec<float,2>") {
				return boost::shared_ptr<Filter>(new AttributeFilter<Vf2, KeepEqual>(attributeName, attributeKind, activeFly, passiveFly, realSpinBox->value(), dimension));
			} else if (attributeType == "float") {
				return boost::shared_ptr<Filter>(new AttributeFilter<float, KeepEqual>(attributeName, attributeKind, activeFly, passiveFly, realSpinBox->value()));
			} else if (attributeType == "uint32_t") {
				return boost::shared_ptr<Filter>(new AttributeFilter<uint32_t, KeepEqual>(attributeName, attributeKind, activeFly, passiveFly, intSpinBox->value()));
			} else if (attributeType == "MyBool") {
				return boost::shared_ptr<Filter>(new AttributeFilter<MyBool, KeepEqual>(attributeName, attributeKind, activeFly, passiveFly, intSpinBox->value()));
			}
		} else if (operatorComboBox->currentText() == "<") {
			if (attributeType == "Vec<float,2>") {
				return boost::shared_ptr<Filter>(new AttributeFilter<Vf2, KeepLessThan>(attributeName, attributeKind, activeFly, passiveFly, realSpinBox->value(), dimension));
			} else if (attributeType == "float") {
				return boost::shared_ptr<Filter>(new AttributeFilter<float, KeepLessThan>(attributeName, attributeKind, activeFly, passiveFly, realSpinBox->value()));
			} else if (attributeType == "uint32_t") {
				return boost::shared_ptr<Filter>(new AttributeFilter<uint32_t, KeepLessThan>(attributeName, attributeKind, activeFly, passiveFly, intSpinBox->value()));
			} else if (attributeType == "MyBool") {
				return boost::shared_ptr<Filter>(new AttributeFilter<MyBool, KeepLessThan>(attributeName, attributeKind, activeFly, passiveFly, intSpinBox->value()));
			}
		} else if (operatorComboBox->currentText() == ">") {
			if (attributeType == "Vec<float,2>") {
				return boost::shared_ptr<Filter>(new AttributeFilter<Vf2, KeepGreaterThan>(attributeName, attributeKind, activeFly, passiveFly, realSpinBox->value(), dimension));
			} else if (attributeType == "float") {
				return boost::shared_ptr<Filter>(new AttributeFilter<float, KeepGreaterThan>(attributeName, attributeKind, activeFly, passiveFly, realSpinBox->value()));
			} else if (attributeType == "uint32_t") {
				return boost::shared_ptr<Filter>(new AttributeFilter<uint32_t, KeepGreaterThan>(attributeName, attributeKind, activeFly, passiveFly, intSpinBox->value()));
			} else if (attributeType == "MyBool") {
				return boost::shared_ptr<Filter>(new AttributeFilter<MyBool, KeepGreaterThan>(attributeName, attributeKind, activeFly, passiveFly, intSpinBox->value()));
			}
		}
	}

	std::cerr << "WARNING: returning NULL Filter in FilterSelector::getFilter()" << std::endl;
	return boost::shared_ptr<Filter>();
}

void FilterSelector::reset()
{
	attributeComboBox->clear();
	attributeComboBox->addItem("<attribute>", QVariant());
	attributeComboBox->insertSeparator(attributeComboBox->count());
	{
		FrameAttributes attributes;
		std::vector<std::string> attributeNames = attributes.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			QStringList attributeInfo;
			attributeInfo.append("frame");
			attributeInfo.append(QString::fromStdString(attributes.get(*iter).getType()));
			attributeComboBox->addItem(QString::fromStdString(*iter), attributeInfo);
		}
	}
	attributeComboBox->insertSeparator(attributeComboBox->count());
	{
		FlyAttributes attributes;
		std::vector<std::string> attributeNames = attributes.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			QStringList attributeInfo;
			attributeInfo.append("fly");
			attributeInfo.append(QString::fromStdString(attributes.get(*iter).getType()));
			attributeComboBox->addItem(QString::fromStdString(*iter), attributeInfo);
		}
	}
	attributeComboBox->insertSeparator(attributeComboBox->count());
	{
		PairAttributes attributes;
		std::vector<std::string> attributeNames = attributes.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			QStringList attributeInfo;
			attributeInfo.append("pair");
			attributeInfo.append(QString::fromStdString(attributes.get(*iter).getType()));
			attributeComboBox->addItem(QString::fromStdString(*iter), attributeInfo);
		}
	}

	dimensionComboBox->clear();
	dimensionComboBox->addItem("<dimension>", QVariant());
	dimensionComboBox->insertSeparator(dimensionComboBox->count());
	activeFlyComboBox->addItem("X", 0u);
	activeFlyComboBox->addItem("Y", 1u);
	dimensionComboBox->hide();

	activeFlyComboBox->clear();
	activeFlyComboBox->addItem("<active fly>", QVariant());
	activeFlyComboBox->insertSeparator(activeFlyComboBox->count());
	activeFlyComboBox->addItem("0", 0u);
	activeFlyComboBox->addItem("1", 1u);
	activeFlyComboBox->hide();

	passiveFlyComboBox->clear();
	passiveFlyComboBox->addItem("<passive fly>", QVariant());
	passiveFlyComboBox->insertSeparator(passiveFlyComboBox->count());
	passiveFlyComboBox->addItem("0", 0u);
	passiveFlyComboBox->addItem("1", 1u);
	passiveFlyComboBox->hide();

	operatorComboBox->clear();
	operatorComboBox->addItem("<operator>", QVariant());
	operatorComboBox->addItem("=", QVariant());
	operatorComboBox->addItem("<", QVariant());
	operatorComboBox->addItem(">", QVariant());
	operatorComboBox->insertSeparator(operatorComboBox->count());
	//operatorComboBox->hide();

	intSpinBox->clear();
	intSpinBox->hide();

	realSpinBox->clear();
	realSpinBox->hide();

	unitComboBox->clear();
	unitComboBox->addItem("<unit>", QVariant());
	unitComboBox->insertSeparator(unitComboBox->count());
	unitComboBox->hide();
}

void FilterSelector::attributeChanged()
{
	int comboBoxIndex = attributeComboBox->currentIndex();
	if (comboBoxIndex < 0) {
		return;
	}

	QString attributeName = attributeComboBox->itemText(comboBoxIndex);
	QVariant itemData = attributeComboBox->itemData(comboBoxIndex);
	if (!itemData.isValid()) {
		return;
	}

	if (itemData.canConvert(QVariant::StringList)) {
		QStringList attributeInfo = itemData.toStringList();
		assert(attributeInfo.size() == 2);
		QString attributeKind = attributeInfo[0];
		QString attributeType = attributeInfo[1];

		dimensionComboBox->setVisible(attributeType == "Vec<float,2>");
		activeFlyComboBox->setVisible(attributeKind == "fly" || attributeKind == "pair");
		passiveFlyComboBox->setVisible(attributeKind == "pair");
		intSpinBox->setVisible(attributeType == "MyBool" || attributeType == "uint32_t");
		realSpinBox->setVisible(attributeType == "float" || attributeType == "Vec<float,2>");
		if (attributeKind == "frame") {
			return;
		}
		if (attributeKind == "fly") {
			return;
		}
		if (attributeKind == "pair") {	//TODO: handle other fly counts
			return;
		}
	}
}
/*
void FilterSelector::dimensionChanged(int index)
{
	if (index < 0) {
		return;
	}

	int comboBoxIndex = attributeComboBox->currentIndex();
	if (comboBoxIndex < 0) {
		return;
	}

	QString attributeName = attributeComboBox->itemText(comboBoxIndex);
	QVariant itemData = attributeComboBox->itemData(comboBoxIndex);
	if (!itemData.isValid()) {
		return;
	}

	if (itemData.canConvert<QString>()) {
		QString kindOfAttribute = itemData.toString();
		if (kindOfAttribute == "frame") {
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			if (currentResults->hasFrameAttribute<Vf2>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<Vf2>& data = currentResults->getFrameData<Vf2>(attributeName.toStdString());
				std::vector<float> xOnly;
				xOnly.reserve(data.size());
				for (size_t i = 0; i != data.size(); ++i) {
					xOnly.push_back(data[i][index]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
			}
			return;
		}
		if (kindOfAttribute == "fly") {
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			if (currentResults->hasFlyAttribute<Vf2>(attributeName.toStdString())) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<Vf2>& data = currentResults->getFlyData<Vf2>(flyNumber, attributeName.toStdString());
					std::vector<float> xOnly;
					xOnly.reserve(data.size());
					for (size_t i = 0; i != data.size(); ++i) {
						xOnly.push_back(data[i][index]);
					}
					plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), "TODO:unit");	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
				}
			}
			return;
		}
		if (kindOfAttribute == "pair" && currentResults->getFlyCount() == 2) {	//TODO: handle other fly counts
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			if (currentResults->hasPairAttribute<Vf2>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<Vf2>& fly0Data = currentResults->getPairData<Vf2>(0, 1, attributeName.toStdString());
				std::vector<float> xOnly;
				xOnly.reserve(fly0Data.size());
				for (size_t i = 0; i != fly0Data.size(); ++i) {
					xOnly.push_back(fly0Data[i][index]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				const Attribute<Vf2>& fly1Data = currentResults->getPairData<Vf2>(1, 0, attributeName.toStdString());
				xOnly.clear();
				for (size_t i = 0; i != fly1Data.size(); ++i) {
					xOnly.push_back(fly1Data[i][index]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
			}
			return;
		}
	}
}

*/