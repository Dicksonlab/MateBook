#include <QtGui>

#include <cmath>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <limits>
#include "Heatmapper.hpp"
#include "ArenaItem.hpp"
#include "VerticalWidgetList.hpp"
#include "FilterSelector.hpp"
#include "../../tracker/source/FrameAttributes.hpp"
#include "../../tracker/source/FlyAttributes.hpp"
#include "../../tracker/source/PairAttributes.hpp"
#include "../../common/source/mathematics.hpp"

// helper function for the VerticalWidgetList
QWidget* createFilter()
{
	return new FilterSelector;
}

Heatmapper::Heatmapper(QWidget* parent) : QWidget(parent)
{
	QVBoxLayout* verticalLayout = new QVBoxLayout;

	{	// what attribute to plot
		QHBoxLayout* horizontalLayout = new QHBoxLayout;
		attributeComboBox = new QComboBox;
		horizontalLayout->addWidget(attributeComboBox);
		activeFlyComboBox = new QComboBox;
		horizontalLayout->addWidget(activeFlyComboBox);
		passiveFlyComboBox = new QComboBox;
		horizontalLayout->addWidget(passiveFlyComboBox);
		verticalLayout->addLayout(horizontalLayout);
	}

	{	// image size
		QHBoxLayout* horizontalLayout = new QHBoxLayout;
		horizontalLayout->addWidget(new QLabel("Image Width:"));
		imageWidthSpinBox = new QSpinBox;
		imageWidthSpinBox->setRange(1, std::numeric_limits<int>::max());
		imageWidthSpinBox->setValue(400);
		horizontalLayout->addWidget(imageWidthSpinBox);

		horizontalLayout->addWidget(new QLabel("Image Height:"));
		imageHeightSpinBox = new QSpinBox;
		imageHeightSpinBox->setRange(1, std::numeric_limits<int>::max());
		imageHeightSpinBox->setValue(400);
		horizontalLayout->addWidget(imageHeightSpinBox);
		verticalLayout->addLayout(horizontalLayout);
	}

	{	// number of bins
		QHBoxLayout* horizontalLayout = new QHBoxLayout;
		horizontalLayout->addWidget(new QLabel("Horizontal Bins:"));
		horizontalBinsSpinBox = new QSpinBox;
		horizontalBinsSpinBox->setRange(1, std::numeric_limits<int>::max());
		horizontalBinsSpinBox->setValue(40);
		horizontalLayout->addWidget(horizontalBinsSpinBox);

		horizontalLayout->addWidget(new QLabel("Vertical Bins:"));
		verticalBinsSpinBox = new QSpinBox;
		verticalBinsSpinBox->setRange(1, std::numeric_limits<int>::max());
		verticalBinsSpinBox->setValue(40);
		horizontalLayout->addWidget(verticalBinsSpinBox);
		verticalLayout->addLayout(horizontalLayout);
	}

	{	// value-to-bin scale factors
		QHBoxLayout* horizontalLayout = new QHBoxLayout;
		horizontalLayout->addWidget(new QLabel("Horizontal Scale Factor:"));
		horizontalScaleFactorSpinBox = new QDoubleSpinBox;
		horizontalScaleFactorSpinBox->setRange(0, std::numeric_limits<double>::max());
		horizontalScaleFactorSpinBox->setSpecialValueText("Auto");
		horizontalScaleFactorSpinBox->setValue(0);
		horizontalLayout->addWidget(horizontalScaleFactorSpinBox);

		horizontalLayout->addWidget(new QLabel("Vertical Scale Factor:"));
		verticalScaleFactorSpinBox = new QDoubleSpinBox;
		verticalScaleFactorSpinBox->setRange(0, std::numeric_limits<double>::max());
		verticalScaleFactorSpinBox->setSpecialValueText("Auto");
		verticalScaleFactorSpinBox->setValue(0);
		horizontalLayout->addWidget(verticalScaleFactorSpinBox);
		verticalLayout->addLayout(horizontalLayout);
	}

	{	// value to display in the center of the heatmap
		QHBoxLayout* horizontalLayout = new QHBoxLayout;
		horizontalLayout->addWidget(new QLabel("Centered X:"));
		centerXSpinBox = new QDoubleSpinBox;
		centerXSpinBox->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
		centerXSpinBox->setValue(0);
		horizontalLayout->addWidget(centerXSpinBox);

		horizontalLayout->addWidget(new QLabel("Centered Y:"));
		centerYSpinBox = new QDoubleSpinBox;
		centerYSpinBox->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
		centerYSpinBox->setValue(0);
		horizontalLayout->addWidget(centerYSpinBox);
		verticalLayout->addLayout(horizontalLayout);
	}

	{	// min and max counts (below and above which we saturate)
		QHBoxLayout* horizontalLayout = new QHBoxLayout;
		horizontalLayout->addWidget(new QLabel("Lowest Count:"));
		lowestCountSpinBox = new QSpinBox;
		lowestCountSpinBox->setRange(-1, std::numeric_limits<int>::max());
		lowestCountSpinBox->setSpecialValueText("Auto");
		lowestCountSpinBox->setValue(-1);
		horizontalLayout->addWidget(lowestCountSpinBox);

		horizontalLayout->addWidget(new QLabel("Highest Count:"));
		highestCountSpinBox = new QSpinBox;
		highestCountSpinBox->setRange(-1, std::numeric_limits<int>::max());
		highestCountSpinBox->setSpecialValueText("Auto");
		highestCountSpinBox->setValue(-1);
		horizontalLayout->addWidget(highestCountSpinBox);
		verticalLayout->addLayout(horizontalLayout);
	}

	{	// color map
		QHBoxLayout* horizontalLayout = new QHBoxLayout;
		horizontalLayout->addWidget(new QLabel("Color Map:"));
		colorMapComboBox = new QComboBox;
		colorMapComboBox->addItem("heat");
		colorMapComboBox->addItem("jet");
		horizontalLayout->addWidget(colorMapComboBox);

		horizontalLayout->addWidget(new QLabel("Mapping:"));
		mappingComboBox = new QComboBox;
		mappingComboBox->addItem("linear");
		mappingComboBox->addItem("logarithmic");
		horizontalLayout->addWidget(mappingComboBox);
		verticalLayout->addLayout(horizontalLayout);
	}

	filterList = new VerticalWidgetList(createFilter);
	filterList->setWidgetCount(1);
	verticalLayout->addWidget(filterList);

	setLayout(verticalLayout);

	reset();

//	connect(attributeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(attributeChanged()));
//	connect(dimensionComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(dimensionChanged(int)));
}

Heatmapper::~Heatmapper()
{
}

QSize Heatmapper::minimumSizeHint() const
{
	return QSize(160, 100);
}

QSize Heatmapper::sizeHint() const
{
	return QSize(340, 100);
}

QImage Heatmapper::drawHeatmap(const std::vector<ArenaItem*>& arenaItems)
{
	std::vector<boost::shared_ptr<Filter> > filters;
	for (int i = 0; i < filterList->count(); ++i) {
		FilterSelector* filterSelector = assert_cast<FilterSelector*>(filterList->getWidget(i));
		boost::shared_ptr<Filter> filter = filterSelector->getFilter();
		if (filter) {
			filters.push_back(filter);
		}
	}

	std::vector<Vf2> allValues;

	for (std::vector<ArenaItem*>::const_iterator arenaIter = arenaItems.begin(); arenaIter != arenaItems.end(); ++arenaIter) {
		// load attribute that will be heatmapped
		QString attributePath = (*arenaIter)->absoluteDataDirectory().filePath(getAttributePath());
		Attribute<Vf2> attribute;
		attribute.readBinaries(attributePath.toStdString());

		// create a vector of all indexes and apply all filters
		size_t attributeSize = attribute.size();
		std::vector<size_t> indexes;
		for (size_t index = 0; index != attributeSize; ++index) {
			indexes.push_back(index);
		}
		for (std::vector<boost::shared_ptr<Filter> >::const_iterator filterIter = filters.begin(); filterIter != filters.end(); ++filterIter) {
			indexes = (*filterIter)->apply(*(*arenaIter), indexes);
		}

		// select the attribute values based on all indexes that made it through the filters
		for (std::vector<size_t>::const_iterator indexIter = indexes.begin(); indexIter != indexes.end(); ++indexIter) {
			allValues.push_back(attribute[*indexIter]);
		}
	}

	// find overall min and max x,y data values to be shown in the heatmap
	float minX = std::numeric_limits<float>::infinity();
	float maxX = -std::numeric_limits<float>::infinity();
	float minY = std::numeric_limits<float>::infinity();
	float maxY = -std::numeric_limits<float>::infinity();
	for (std::vector<Vf2>::const_iterator iter = allValues.begin(); iter != allValues.end(); ++iter) {
		minX = std::min(minX, (*iter)[0]);
		maxX = std::max(maxX, (*iter)[0]);
		minY = std::min(minY, (*iter)[1]);
		maxY = std::max(maxY, (*iter)[1]);
	}
	// increase the range to put the given center in the middle
	if (centerXSpinBox->value() - minX > maxX - centerXSpinBox->value()) {
		maxX = centerXSpinBox->value() + (centerXSpinBox->value() - minX);
	}
	if (centerXSpinBox->value() - minX < maxX - centerXSpinBox->value()) {
		minX = centerXSpinBox->value() - (maxX - centerXSpinBox->value());
	}
	if (centerYSpinBox->value() - minY > maxY - centerYSpinBox->value()) {
		maxY = centerYSpinBox->value() + (centerYSpinBox->value() - minY);
	}
	if (centerYSpinBox->value() - minY < maxY - centerYSpinBox->value()) {
		minY = centerYSpinBox->value() - (maxY - centerYSpinBox->value());
	}

	// if the user provided a scale factor, use it instead
	if (horizontalScaleFactorSpinBox->value()) {
		minX = centerXSpinBox->value() - horizontalScaleFactorSpinBox->value() * static_cast<double>(horizontalBinsSpinBox->value()) / 2;
		maxX = centerXSpinBox->value() + horizontalScaleFactorSpinBox->value() * static_cast<double>(horizontalBinsSpinBox->value()) / 2;
	}
	if (verticalScaleFactorSpinBox->value()) {
		minY = centerYSpinBox->value() - verticalScaleFactorSpinBox->value() * static_cast<double>(verticalBinsSpinBox->value()) / 2;
		maxY = centerYSpinBox->value() + verticalScaleFactorSpinBox->value() * static_cast<double>(verticalBinsSpinBox->value()) / 2;
	}

	if (minX >= maxX || minY >= maxY ||
		std::abs(minX) == std::numeric_limits<float>::infinity() ||
		std::abs(maxX) == std::numeric_limits<float>::infinity() ||
		std::abs(minY) == std::numeric_limits<float>::infinity() ||
		std::abs(maxY) == std::numeric_limits<float>::infinity() ||
		isNaN(minX) || isNaN(maxX) || isNaN(minY) || isNaN(maxY))
	{
		QMessageBox::warning(this, tr("Drawing heatmap"), tr("Could not map the range of values to an image:\nX-range: [%1, %2]\nY-range: [%3, %4]").arg(minX).arg(maxX).arg(minY).arg(maxY));
		return QImage();
	}

	int horizontalBins = horizontalBinsSpinBox->value();
	int verticalBins = verticalBinsSpinBox->value();

	float binSizeX = (maxX - minX) / horizontalBins;
	float binSizeY = (maxY - minY) / verticalBins;

	std::vector<size_t> counts(horizontalBins * verticalBins, 0);
	for (std::vector<Vf2>::const_iterator iter = allValues.begin(); iter != allValues.end(); ++iter) {
		int binX = ((*iter)[0] - minX) / binSizeX;
		int binY = ((*iter)[1] - minY) / binSizeY;
		if (binX >= 0 && binX < horizontalBins && binY >= 0 && binY < verticalBins) {
			++counts[horizontalBins * binY + binX];
		}
	}

	// find min and max counts
	size_t minCount = std::numeric_limits<size_t>::max();
	size_t maxCount = 0;
	for (std::vector<size_t>::const_iterator iter = counts.begin(); iter != counts.end(); ++iter) {
		minCount = std::min(minCount, (*iter));
		maxCount = std::max(maxCount, (*iter));
	}

	// if the user provided min or max counts, use those instead
	if (lowestCountSpinBox->value() >= 0) {
		minCount = lowestCountSpinBox->value();
	}
	if (highestCountSpinBox->value() >= 0) {
		maxCount = highestCountSpinBox->value();
	}

	QImage heatmap(horizontalBins, verticalBins, QImage::Format_RGB32);
	heatmap.fill(Qt::black);
	if (maxCount <= minCount) {
		return heatmap.scaled(imageWidthSpinBox->value(), imageHeightSpinBox->value());
	}

	const int rangeMin = 0;
	const int rangeMax = 255;

	const float minCountFloat = static_cast<float>(minCount);
	const float maxCountFloat = static_cast<float>(maxCount);
	const float minCountLog = std::log10(minCountFloat + 1);
	const float maxCountLog = std::log10(maxCountFloat + 1);
	const bool logarithmicMapping = (mappingComboBox->currentText() == "logarithmic");
	for (int x = 0; x < horizontalBins; ++x) {
		for (int y = 0; y < verticalBins; ++y) {
			float thisCount = counts[horizontalBins * y + x];
			thisCount = std::min(thisCount, maxCountFloat);
			thisCount = std::max(thisCount, minCountFloat);
			float normalizedValue = (thisCount - minCountFloat) / (maxCountFloat - minCountFloat);
			if (logarithmicMapping) {
				normalizedValue = (std::log10(thisCount + 1) - minCountLog) / (maxCountLog - minCountLog);
			}
			normalizedValue *= (rangeMax - rangeMin);
			normalizedValue += rangeMin;
			int normalizedInt = static_cast<int>(normalizedValue);
			normalizedInt = std::max(normalizedInt, rangeMin);
			normalizedInt = std::min(normalizedInt, rangeMax);
			heatmap.setPixel(x, y, qRgb(normalizedInt, 0, 0));
		}
	}

	return heatmap.scaled(imageWidthSpinBox->value(), imageHeightSpinBox->value());
}

void Heatmapper::reset()
{
	attributeComboBox->clear();
	attributeComboBox->addItem("<attribute>", QVariant());
	attributeComboBox->insertSeparator(attributeComboBox->count());
	{
		FrameAttributes attributes;
		std::vector<std::string> attributeNames = attributes.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			try {
				Attribute<Vf2>& attribute = dynamic_cast<Attribute<Vf2>&>(attributes.get(*iter));
				attributeComboBox->addItem(QString::fromStdString(*iter), "frame");
			} catch (std::bad_cast) {
			}
		}
	}
	attributeComboBox->insertSeparator(attributeComboBox->count());
	{
		FlyAttributes attributes;
		std::vector<std::string> attributeNames = attributes.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			try {
				Attribute<Vf2>& attribute = dynamic_cast<Attribute<Vf2>&>(attributes.get(*iter));
				attributeComboBox->addItem(QString::fromStdString(*iter), "fly");
			} catch (std::bad_cast) {
			}
		}
	}
	attributeComboBox->insertSeparator(attributeComboBox->count());
	{
		PairAttributes attributes;
		std::vector<std::string> attributeNames = attributes.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			try {
				Attribute<Vf2>& attribute = dynamic_cast<Attribute<Vf2>&>(attributes.get(*iter));
				attributeComboBox->addItem(QString::fromStdString(*iter), "pair");
			} catch (std::bad_cast) {
			}
		}
	}

	activeFlyComboBox->clear();
	activeFlyComboBox->addItem("<active fly>", QVariant());
	activeFlyComboBox->insertSeparator(activeFlyComboBox->count());
	activeFlyComboBox->addItem("0", 0u);
	activeFlyComboBox->addItem("1", 1u);

	passiveFlyComboBox->clear();
	passiveFlyComboBox->addItem("<passive fly>", QVariant());
	passiveFlyComboBox->insertSeparator(passiveFlyComboBox->count());
	passiveFlyComboBox->addItem("0", 0u);
	passiveFlyComboBox->addItem("1", 1u);
}
/*
void Heatmapper::attributeChanged()
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

	if (itemData.canConvert<QString>()) {
		dimensionComboBox->clear();
		QString kindOfAttribute = itemData.toString();
		if (kindOfAttribute == "frame") {
			plot->clear();
			plot->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
			//TODO: make grapher handle the different attribute types
			QString attributeDescription("Cannot be displayed.");
			if (currentResults->hasFrameAttribute<float>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<float>& data = currentResults->getFrameData<float>(attributeName.toStdString());
				plot->addLineFillGraph(data.begin(), data.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				attributeDescription = QString::fromStdString(data.getDescription());
			} else if (currentResults->hasFrameAttribute<MyBool>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<MyBool>& data = currentResults->getFrameData<MyBool>(attributeName.toStdString());
				plot->addLineFillGraph(data.begin(), data.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				attributeDescription = QString::fromStdString(data.getDescription());
			} else if (currentResults->hasFrameAttribute<uint32_t>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<uint32_t>& data = currentResults->getFrameData<uint32_t>(attributeName.toStdString());
				plot->addLineFillGraph(data.begin(), data.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				attributeDescription = QString::fromStdString(data.getDescription());
			} else if (currentResults->hasFrameAttribute<Vf2>(attributeName.toStdString())) {
				dimensionComboBox->blockSignals(true);
				dimensionComboBox->addItem("X", 0);
				dimensionComboBox->addItem("Y", 1);
				dimensionComboBox->blockSignals(false);
				//TODO: check if the data is actually available
				const Attribute<Vf2>& data = currentResults->getFrameData<Vf2>(attributeName.toStdString());
				std::vector<float> xOnly;
				xOnly.reserve(data.size());
				for (size_t i = 0; i != data.size(); ++i) {
					xOnly.push_back(data[i][0]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), QVector4D(0, 0, 0, 1), 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
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
			if (currentResults->hasFlyAttribute<float>(attributeName.toStdString())) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<float>& data = currentResults->getFlyData<float>(flyNumber, attributeName.toStdString());
					plot->addLineFillGraph(data.begin(), data.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0, -0.1 * flyNumber - 0.1), "TODO:unit");	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
					attributeDescription = QString::fromStdString(data.getDescription());
				}
			} else if (currentResults->hasFlyAttribute<MyBool>(attributeName.toStdString())) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<MyBool>& data = currentResults->getFlyData<MyBool>(flyNumber, attributeName.toStdString());
					plot->addLineFillGraph(data.begin(), data.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), "TODO:unit");	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
					attributeDescription = QString::fromStdString(data.getDescription());
				}
				frame->setTitle("Fly attribute " + attributeName + " shown");
			} else if (currentResults->hasFlyAttribute<uint32_t>(attributeName.toStdString())) {
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<uint32_t>& data = currentResults->getFlyData<uint32_t>(flyNumber, attributeName.toStdString());
					plot->addLineFillGraph(data.begin(), data.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), "TODO:unit");	//TODO: fix grapher near- and far-plane
//					flyComboBox->addItem(QString("fly ") + QString::number(flyNumber), QVariant());
					attributeDescription = QString::fromStdString(data.getDescription());
				}
			} else if (currentResults->hasFlyAttribute<Vf2>(attributeName.toStdString())) {
				dimensionComboBox->blockSignals(true);
				dimensionComboBox->addItem("X", 0);
				dimensionComboBox->addItem("Y", 1);
				dimensionComboBox->blockSignals(false);
				for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
					//TODO: check if the data is actually available
					const Attribute<Vf2>& data = currentResults->getFlyData<Vf2>(flyNumber, attributeName.toStdString());
					std::vector<float> xOnly;
					xOnly.reserve(data.size());
					for (size_t i = 0; i != data.size(); ++i) {
						xOnly.push_back(data[i][0]);
					}
					plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[flyNumber], 0.5, 1, QVector3D(currentResults->getOffset(), 0,  -0.1 * flyNumber - 0.1), "TODO:unit");	//TODO: fix grapher near- and far-plane
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
			if (currentResults->hasPairAttribute<float>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<float>& fly0Data = currentResults->getPairData<float>(0, 1, attributeName.toStdString());
				plot->addLineFillGraph(fly0Data.begin(), fly0Data.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				const Attribute<float>& fly1Data = currentResults->getPairData<float>(1, 0, attributeName.toStdString());
				plot->addLineFillGraph(fly1Data.begin(), fly1Data.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			} else if (currentResults->hasPairAttribute<MyBool>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<MyBool>& fly0Data = currentResults->getPairData<MyBool>(0, 1, attributeName.toStdString());
				plot->addLineFillGraph(fly0Data.begin(), fly0Data.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				const Attribute<MyBool>& fly1Data = currentResults->getPairData<MyBool>(1, 0, attributeName.toStdString());
				plot->addLineFillGraph(fly1Data.begin(), fly1Data.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			} else if (currentResults->hasPairAttribute<uint32_t>(attributeName.toStdString())) {
				//TODO: check if the data is actually available
				const Attribute<uint32_t>& fly0Data = currentResults->getPairData<uint32_t>(0, 1, attributeName.toStdString());
				plot->addLineFillGraph(fly0Data.begin(), fly0Data.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				const Attribute<uint32_t>& fly1Data = currentResults->getPairData<uint32_t>(1, 0, attributeName.toStdString());
				plot->addLineFillGraph(fly1Data.begin(), fly1Data.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			} else if (currentResults->hasPairAttribute<Vf2>(attributeName.toStdString())) {
				dimensionComboBox->blockSignals(true);
				dimensionComboBox->addItem("X", 0);
				dimensionComboBox->addItem("Y", 1);
				dimensionComboBox->blockSignals(false);
				//TODO: check if the data is actually available
				const Attribute<Vf2>& fly0Data = currentResults->getPairData<Vf2>(0, 1, attributeName.toStdString());
				std::vector<float> xOnly;
				xOnly.reserve(fly0Data.size());
				for (size_t i = 0; i != fly0Data.size(); ++i) {
					xOnly.push_back(fly0Data[i][0]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[0], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				const Attribute<Vf2>& fly1Data = currentResults->getPairData<Vf2>(1, 0, attributeName.toStdString());
				xOnly.clear();
				for (size_t i = 0; i != fly1Data.size(); ++i) {
					xOnly.push_back(fly1Data[i][0]);
				}
				plot->addLineFillGraph(xOnly.begin(), xOnly.end(), flyColors[1], 0.5, 1, QVector3D(currentResults->getOffset(), 0, 0), "TODO:unit");
				attributeDescription = QString::fromStdString(fly0Data.getDescription());
			}
			frame->setTitle("Pair attribute: " + attributeDescription);
			return;
		}
	}
}

void Heatmapper::dimensionChanged(int index)
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

QString Heatmapper::getAttributePath() const
{
	QVariant itemData = attributeComboBox->itemData(attributeComboBox->currentIndex());
	if (itemData.isValid() && itemData.canConvert(QVariant::String)) {
		QString attributeKind = itemData.toString();
		QString attributeName = attributeComboBox->currentText();
		
		unsigned int activeFly = 0;
		if (attributeKind == "fly" || attributeKind == "pair") {
			QVariant activeData = activeFlyComboBox->itemData(activeFlyComboBox->currentIndex());
			if (activeData.isValid() && activeData.canConvert(QVariant::UInt)) {
				activeFly = activeData.toUInt();
			} else {
				std::cerr << "cannot create path in Heatmapper::getAttributePath(): active fly not selected" << std::endl;
				return "";
			}
		}

		unsigned int passiveFly = 0;
		if (attributeKind == "pair") {
			QVariant passiveData = passiveFlyComboBox->itemData(passiveFlyComboBox->currentIndex());
			if (passiveData.isValid() && passiveData.canConvert(QVariant::UInt)) {
				passiveFly = passiveData.toUInt();
			} else {
				std::cerr << "cannot create path in Heatmapper::getAttributePath(): passive fly not selected" << std::endl;
				return "";
			}
		}

		QString path("track/");
		path += attributeKind + "/";
		path += (attributeKind == "fly" || attributeKind == "pair") ? (QString::number(activeFly) + "/") : "";
		path += (attributeKind == "pair") ? (QString::number(passiveFly) + "/") : "";
		path += attributeName;
		return path;
	}

	std::cerr << "cannot create path in Heatmapper::getAttributePath(): attribute not selected" << std::endl;
	return "";
}
