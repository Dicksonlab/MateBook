#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QVector4D>
#include <QGroupBox>
#include <QComboBox>
#include <QStringList>
#include <QMessageBox>
#include <QLabel>

#include <cmath>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "SongStatisticsViewer.hpp"

SongStatisticsViewer::SongStatisticsViewer(QWidget* parent) : QWidget(parent),
data()
{
	plot = new QGLGrapher("", this);
	plot->setAntiAliasing(true);
	plot->setFontSize(20);
	plot->setAutoScale(true);

	QHBoxLayout* layout = new QHBoxLayout;
	frame = new QGroupBox();
	
	ipiDisplayMode = new QComboBox();
	
	userInput = new QLabel("");
	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setBackgroundRole(QPalette::Light);
	scrollArea->setWidget(userInput);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFixedWidth(160);

	QVBoxLayout* plotExpl = new QVBoxLayout;
	plotExpl->addWidget(ipiDisplayMode);
	plotExpl->addWidget(scrollArea);

	QHBoxLayout* plotAndComboBoxLayout = new QHBoxLayout;
	plotAndComboBoxLayout->addWidget(plot, 10);
	plotAndComboBoxLayout->addLayout(plotExpl, 1);

	frame->setLayout(plotAndComboBoxLayout);
	layout->addWidget(frame);
	setLayout(layout);

	connect(plot, SIGNAL(centerChanged(size_t)), this, SIGNAL(plotCenterChanged(size_t)));
	connect(plot, SIGNAL(numValuesChanged(size_t)), this, SIGNAL(plotZoomChanged(size_t)));

	connect(ipiDisplayMode, SIGNAL(currentIndexChanged(int)), this, SLOT(displayModeChanged(int)));
}

SongStatisticsViewer::~SongStatisticsViewer()
{
}

QSize SongStatisticsViewer::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize SongStatisticsViewer::sizeHint() const
{
	return QSize(640, 120);
}

void SongStatisticsViewer::setGraph(const std::vector< SongResults::binStruct >& data, const QStringList& names)
{
	ipiDisplayMode->clear();
	this->data = data;
	if(names.size() == data.size()){
		for(int i = 0; i < names.size();  ++i){
			ipiDisplayMode->addItem(names[i]);
		}
	}else{
		QMessageBox::critical(this, tr("Drawing Graph:"), "There is not the same number of graphs and graph names");
	}
}

void SongStatisticsViewer::clear()
{
	plot->clear();
	ipiDisplayMode->clear();
	userInput->clear();
}

void SongStatisticsViewer::setCurrentResults()
{
	clear();
}

void SongStatisticsViewer::center(size_t frame)
{
	plot->center(frame);
}

void SongStatisticsViewer::setNumValues(size_t numValues)
{
	plot->setNumValues(numValues);
}

void SongStatisticsViewer::setAntiAliasing(bool enable)
{
	plot->setAntiAliasing(enable);
}

void SongStatisticsViewer::setRotated(bool rotated)
{
	plot->setRotated(rotated);
}

void SongStatisticsViewer::setFontSize(int pixel)
{
	plot->setFontSize(pixel);
}

void SongStatisticsViewer::displayModeChanged(int index)
{
	plot->clear();
	if (index < 0 && index > data.size()) {
		return;
	}

	frame->setTitle(ipiDisplayMode->currentText());

	size_t dataSize = data[index].size();
	if(dataSize > 0){
		std::vector<QVector4D> colors(dataSize);
		for(int i = 0; i < dataSize; ++i){
			colors[i] = i%2==0 ? QVector4D(0.52, 0.80, 0.83, 1.0) : QVector4D(0.52, 0.80, 0.92, 1.0);
		}

		float total = 0;
		std::vector<float> graphData(dataSize);
		for(size_t i = 0; i < dataSize; ++i){
			total += data[index].at(i).second;
			graphData[i] = data[index].at(i).second;
		}
		std::stringstream s;

		switch(index){
			case 0:{
				s << ipiDisplayMode->currentText().toStdString() << ": total of " << total << " %\n\n";
				for(size_t i = 0; i < dataSize; ++i){
						s << data[index].at(i).first.first << " - " << data[index].at(i).first.second << " ms: " << data[index].at(i).second << "% \n";
				}
				userInput->setText(QString::fromStdString(s.str()));
				plot->addBarGraph(graphData.begin(), graphData.end(), colors.begin(), 1, QVector3D(0, 0, -0.1), "%");	//TODO: fix grapher near- and far-plane
				break;
			}
			case 1:{
				s << ipiDisplayMode->currentText().toStdString() << ": total of " << total << " %\n\n";
				for(size_t i = 0; i < dataSize; ++i){
					s << "  " << i << " per pulse: " << data[index].at(i).second << " % \n";
				}
				userInput->setText(QString::fromStdString(s.str()));
				plot->addBarGraph(graphData.begin(), graphData.end(), colors.begin(), 1, QVector3D(0, 0, -0.1), "%");	//TODO: fix grapher near- and far-plane
				break;
			}
			case 2:{
				s << ipiDisplayMode->currentText().toStdString() << ": total of " << total << " %\n\n";
				for(size_t i = 0; i < dataSize; ++i){
						s << data[index].at(i).first.first << " - " << data[index].at(i).first.second << " pulses: " << data[index].at(i).second << "% \n";
				}
				userInput->setText(QString::fromStdString(s.str()));
				plot->addBarGraph(graphData.begin(), graphData.end(), colors.begin(), 1, QVector3D(0, 0, -0.1), "%");	//TODO: fix grapher near- and far-plane
				break;
			}
			default:
				userInput->setText(QString(""));
				plot->addLineGraph(graphData.begin(), graphData.end(), colors.begin(), 1, QVector3D(0, 0, -0.1), "%");	//TODO: fix grapher near- and far-plane
				break;
		}

		center(dataSize/2);
		setNumValues(dataSize);
	}
}