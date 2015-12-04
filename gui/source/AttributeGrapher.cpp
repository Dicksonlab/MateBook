#include <QtGui>

#include <cmath>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "AttributeGrapher.hpp"
#include "ArenaItem.hpp"
#include "makeColorMap.hpp"

AttributeGrapher::AttributeGrapher(QWidget* parent) : QWidget(parent)
{
	plot = new QGLGrapher("frame", this);
	plot->setAntiAliasing(true);
	plot->setFontSize(20);
	plot->setAutoScale(true);

//	unitComboBox = new QComboBox;
//	flyComboBox = new QComboBox;
	attributeComboBox = new QComboBox;
	dimensionComboBox = new QComboBox;

	QHBoxLayout* layout = new QHBoxLayout;
	frame = new QGroupBox(tr("choose an attribute to visualize"));

	QHBoxLayout* plotAndComboBoxLayout = new QHBoxLayout;
	plotAndComboBoxLayout->addWidget(plot, 10);

	QVBoxLayout* comboBoxLayout = new QVBoxLayout;
//	comboBoxLayout->addWidget(unitComboBox);
//	comboBoxLayout->addWidget(flyComboBox);
	comboBoxLayout->addWidget(attributeComboBox);
	comboBoxLayout->addWidget(dimensionComboBox);
	plotAndComboBoxLayout->addLayout(comboBoxLayout, 1);

	frame->setLayout(plotAndComboBoxLayout);
	layout->addWidget(frame);
	setLayout(layout);

	connect(plot, SIGNAL(centerChanged(size_t)), this, SIGNAL(plotCenterChanged(size_t)));
	connect(plot, SIGNAL(numValuesChanged(size_t)), this, SIGNAL(plotZoomChanged(size_t)));

	connect(attributeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(attributeChanged()));
	connect(dimensionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(dimensionChanged(int)));
}

AttributeGrapher::~AttributeGrapher()
{
}

QSize AttributeGrapher::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize AttributeGrapher::sizeHint() const
{
	return QSize(640, 120);
}

size_t AttributeGrapher::getNumValues() const
{
	return plot->getNumValues();
}

size_t AttributeGrapher::getCenter() const
{
	return plot->getCenter();
}

void AttributeGrapher::reset()
{
	plot->clear();
//	unitComboBox->clear();
//	flyComboBox->clear();
	attributeComboBox->clear();
	attributeComboBox->addItem("<attributes>", QVariant());
	attributeComboBox->insertSeparator(attributeComboBox->count());
}

void AttributeGrapher::setCurrentResults(boost::shared_ptr<TrackingResults> trackingResults)
{
	QString previouslySelected = attributeComboBox->currentText();
	reset();
	if (!trackingResults) {
		return;
	}
	currentResults = trackingResults;
	flyColors = makeColorMap(currentResults->getFlyCount());

	FrameAttributes frameAttributes = FrameAttributes();
  std::vector<std::string> frameAttributeNames = frameAttributes.getNames();
	for (std::vector<std::string>::const_iterator iter = frameAttributeNames.begin(); iter != frameAttributeNames.end(); ++iter) {
    std::string shortname = frameAttributes.get(*iter).getShortName();
    if(shortname.empty())  continue;
    attributeComboBox->addItem(QString::fromStdString(shortname),
          QString::fromStdString("frame,"+*iter));
		if (QString::fromStdString(*iter) == previouslySelected) {
			attributeComboBox->setCurrentIndex(attributeComboBox->count() - 1);
		}
	}

	attributeComboBox->insertSeparator(attributeComboBox->count());

	FlyAttributes flyAttributes = FlyAttributes();
  std::vector<std::string> flyAttributeNames = flyAttributes.getNames();
	for (std::vector<std::string>::const_iterator iter = flyAttributeNames.begin(); iter != flyAttributeNames.end(); ++iter) {
    std::string shortname = flyAttributes.get(*iter).getShortName();
    if(shortname.empty())  continue;
    attributeComboBox->addItem(QString::fromStdString(shortname),
          QString::fromStdString("fly,"+*iter));
		if (QString::fromStdString(*iter) == previouslySelected) {
			attributeComboBox->setCurrentIndex(attributeComboBox->count() - 1);
		}
	}

	attributeComboBox->insertSeparator(attributeComboBox->count());

	PairAttributes pairAttributes = PairAttributes();
  std::vector<std::string> pairAttributeNames = pairAttributes.getNames();
	for (std::vector<std::string>::const_iterator iter = pairAttributeNames.begin(); iter != pairAttributeNames.end(); ++iter) {
    std::string shortname = pairAttributes.get(*iter).getShortName();
    if(shortname.empty())  continue;
    attributeComboBox->addItem(QString::fromStdString(shortname),
          QString::fromStdString("pair,"+*iter));
		if (QString::fromStdString(*iter) == previouslySelected) {
			attributeComboBox->setCurrentIndex(attributeComboBox->count() - 1);
		}
	}
}

void AttributeGrapher::setCenter(size_t frame)
{
	plot->center(frame);
}

void AttributeGrapher::setNumValues(size_t numValues)
{
	plot->setNumValues(numValues);
}

void AttributeGrapher::setAntiAliasing(bool enable)
{
	plot->setAntiAliasing(enable);
}

void AttributeGrapher::setRotated(bool rotated)
{
	plot->setRotated(rotated);
}

void AttributeGrapher::setFontSize(int pixel)
{
	plot->setFontSize(pixel);
}

void AttributeGrapher::attributeChanged()
{
	int comboBoxIndex = attributeComboBox->currentIndex();
	if (comboBoxIndex < 0) {
		return;
	}

	QVariant itemData = attributeComboBox->itemData(comboBoxIndex);
	if (!itemData.isValid()) {
		return;
	}

	if (itemData.canConvert<QString>()) {
		dimensionComboBox->clear();
		QStringList foo = itemData.toString().split(",");
		std::string kindOfAttribute = foo.at(0).toStdString();
		std::string attributeName = foo.at(1).toStdString();
		if (kindOfAttribute == "frame") {
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			//TODO: make grapher handle the different attribute types
			QString attributeDescription("Cannot be displayed.");
			if (currentResults->hasFrameAttribute<float>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<float>& data = currentResults->getFrameData<float>(attributeName);
				plot->addLineFillGraph(data.begin(), data.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), data.getUnit());
				attributeDescription = QString::fromStdString(data.getDescription());
			} else if (currentResults->hasFrameAttribute<MyBool>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<MyBool>& data = currentResults->getFrameData<MyBool>(attributeName);
				plot->addLineFillGraph(data.begin(), data.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), data.getUnit());
				attributeDescription = QString::fromStdString(data.getDescription());
			} else if (currentResults->hasFrameAttribute<uint32_t>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<uint32_t>& data = currentResults->getFrameData<uint32_t>(attributeName);
				plot->addLineFillGraph(data.begin(), data.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), data.getUnit());
				attributeDescription = QString::fromStdString(data.getDescription());
			} else if (currentResults->hasFrameAttribute<Vf2>(attributeName)) {
				dimensionComboBox->blockSignals(true);
				dimensionComboBox->addItem("X", 0);
				dimensionComboBox->addItem("Y", 1);
				dimensionComboBox->blockSignals(false);
				//TODO: check if the data is actually available
				const Attribute<Vf2>& data = currentResults->getFrameData<Vf2>(attributeName);
				std::vector<float> xOnly;
				xOnly.reserve(data.size());
				for (size_t i = 0; i != data.size(); ++i) {
					xOnly.push_back(data[i][0]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), data.getUnit());
				attributeDescription = QString::fromStdString(data.getDescription());
			}
			frame->setTitle("Frame attribute: " + attributeDescription);
			return;
		}
		if (kindOfAttribute == "fly") {
//			flyComboBox->clear();
//			flyComboBox->addItem("all flies", QVariant());
//			flyComboBox->insertSeparator(flyComboBox->count());

			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			//TODO: make grapher handle the different attribute types
			QString attributeDescription("Cannot be displayed.");
			if (currentResults->hasFlyAttribute<float>(attributeName)) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<float>& data = currentResults->getFlyData<float>(flyNumber, attributeName);
					plot->addLineFillGraph(data.begin(), data.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0, -0.1 * flyNumber - 0.1), data.getUnit());	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
					attributeDescription = QString::fromStdString(data.getDescription());
				}
			} else if (currentResults->hasFlyAttribute<MyBool>(attributeName)) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<MyBool>& data = currentResults->getFlyData<MyBool>(flyNumber, attributeName);
					plot->addLineFillGraph(data.begin(), data.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), data.getUnit());	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
					attributeDescription = QString::fromStdString(data.getDescription());
				}
				frame->setTitle("Fly attribute " + QString::fromStdString(attributeName) + " shown");
			} else if (currentResults->hasFlyAttribute<uint32_t>(attributeName)) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<uint32_t>& data = currentResults->getFlyData<uint32_t>(flyNumber, attributeName);
					plot->addLineFillGraph(data.begin(), data.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), data.getUnit());	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
					attributeDescription = QString::fromStdString(data.getDescription());
				}
			} else if (currentResults->hasFlyAttribute<Vf2>(attributeName)) {
				dimensionComboBox->blockSignals(true);
				dimensionComboBox->addItem("X", 0);
				dimensionComboBox->addItem("Y", 1);
				dimensionComboBox->blockSignals(false);
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<Vf2>& data = currentResults->getFlyData<Vf2>(flyNumber, attributeName);
					std::vector<float> xOnly;
					xOnly.reserve(data.size());
					for (size_t i = 0; i != data.size(); ++i) {
						xOnly.push_back(data[i][0]);
					}
					plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), data.getUnit());	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
					attributeDescription = QString::fromStdString(data.getDescription());
				}
			}
			frame->setTitle("Fly attribute: " + attributeDescription);
			return;
		}
		if (kindOfAttribute == "pair" && currentResults->getFlyCount() == 2) {	//TODO: handle other fly counts
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			//TODO: make grapher handle the different attribute types
			QString attributeDescription("Cannot be displayed.");
			if (currentResults->hasPairAttribute<float>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<float>& fly0Data = currentResults->getPairData<float>(0, 1, attributeName);
				plot->addLineFillGraph(fly0Data.begin(), fly0Data.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly0Data.getUnit());
				const Attribute<float>& fly1Data = currentResults->getPairData<float>(1, 0, attributeName);
				plot->addLineFillGraph(fly1Data.begin(), fly1Data.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly1Data.getUnit());
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			} else if (currentResults->hasPairAttribute<MyBool>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<MyBool>& fly0Data = currentResults->getPairData<MyBool>(0, 1, attributeName);
				plot->addLineFillGraph(fly0Data.begin(), fly0Data.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly0Data.getUnit());
				const Attribute<MyBool>& fly1Data = currentResults->getPairData<MyBool>(1, 0, attributeName);
				plot->addLineFillGraph(fly1Data.begin(), fly1Data.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly1Data.getUnit());
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			} else if (currentResults->hasPairAttribute<uint32_t>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<uint32_t>& fly0Data = currentResults->getPairData<uint32_t>(0, 1, attributeName);
				plot->addLineFillGraph(fly0Data.begin(), fly0Data.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly0Data.getUnit());
				const Attribute<uint32_t>& fly1Data = currentResults->getPairData<uint32_t>(1, 0, attributeName);
				plot->addLineFillGraph(fly1Data.begin(), fly1Data.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly1Data.getUnit());
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			} else if (currentResults->hasPairAttribute<Vf2>(attributeName)) {
				dimensionComboBox->blockSignals(true);
				dimensionComboBox->addItem("X", 0);
				dimensionComboBox->addItem("Y", 1);
				dimensionComboBox->blockSignals(false);
				//TODO: check if the data is actually available
				const Attribute<Vf2>& fly0Data = currentResults->getPairData<Vf2>(0, 1, attributeName);
				std::vector<float> xOnly;
				xOnly.reserve(fly0Data.size());
				for (size_t i = 0; i != fly0Data.size(); ++i) {
					xOnly.push_back(fly0Data[i][0]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly0Data.getUnit());
				const Attribute<Vf2>& fly1Data = currentResults->getPairData<Vf2>(1, 0, attributeName);
				xOnly.clear();
				for (size_t i = 0; i != fly1Data.size(); ++i) {
					xOnly.push_back(fly1Data[i][0]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly1Data.getUnit());
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			}
			frame->setTitle("Pair attribute: " + attributeDescription);
			return;
		}
	}
}

void AttributeGrapher::dimensionChanged(int index)
{
	if (index < 0) {
		return;
	}

	int comboBoxIndex = attributeComboBox->currentIndex();
	if (comboBoxIndex < 0) {
		return;
	}

	QVariant itemData = attributeComboBox->itemData(comboBoxIndex);
	if (!itemData.isValid()) {
		return;
	}

	if (itemData.canConvert<QString>()) {
		QStringList foo = itemData.toString().split(",");
		std::string kindOfAttribute = foo.at(0).toStdString();
		std::string attributeName = foo.at(1).toStdString();
		if (kindOfAttribute == "frame") {
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			if (currentResults->hasFrameAttribute<Vf2>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<Vf2>& data = currentResults->getFrameData<Vf2>(attributeName);
				std::vector<float> xOnly;
				xOnly.reserve(data.size());
				for (size_t i = 0; i != data.size(); ++i) {
					xOnly.push_back(data[i][index]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), data.getUnit());
			}
			return;
		}
		if (kindOfAttribute == "fly") {
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			if (currentResults->hasFlyAttribute<Vf2>(attributeName)) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<Vf2>& data = currentResults->getFlyData<Vf2>(flyNumber, attributeName);
					std::vector<float> xOnly;
					xOnly.reserve(data.size());
					for (size_t i = 0; i != data.size(); ++i) {
						xOnly.push_back(data[i][index]);
					}
					plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), data.getUnit());	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
				}
			}
			return;
		}
		if (kindOfAttribute == "pair" && currentResults->getFlyCount() == 2) {	//TODO: handle other fly counts
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			if (currentResults->hasPairAttribute<Vf2>(attributeName)) {
				//TODO: check if the data is actually available
				const Attribute<Vf2>& fly0Data = currentResults->getPairData<Vf2>(0, 1, attributeName);
				std::vector<float> xOnly;
				xOnly.reserve(fly0Data.size());
				for (size_t i = 0; i != fly0Data.size(); ++i) {
					xOnly.push_back(fly0Data[i][index]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly0Data.getUnit());
				const Attribute<Vf2>& fly1Data = currentResults->getPairData<Vf2>(1, 0, attributeName);
				xOnly.clear();
				for (size_t i = 0; i != fly1Data.size(); ++i) {
					xOnly.push_back(fly1Data[i][index]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), fly1Data.getUnit());
			}
			return;
		}
	}
}
