#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QTableView>
#include <QMessageBox>
#include <QComboBox>
#include <QModelIndex>
#include <QSplitter>

#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>
#include <stdexcept>

#include "Project.hpp"
#include "StatisticsTab.hpp"

StatisticsTab::StatisticsTab(QWidget* parent) : AbstractTab(parent)
{
	fileTableModel = new QStandardItemModel(this);
	fileTable = new QTableView(this);
	fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	fileTable->setModel(fileTableModel);

	plotStatistics = new QGLGrapher("", this);
	plotStatistics->setAntiAliasing(true);
	plotStatistics->setFontSize(20);
	plotStatistics->setAutoScale(true);
	plotStatistics->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	binTableModel = new QStandardItemModel(this);
	binTable = new QTableView(this);
	binTable->setModel(binTableModel);
	binTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

	displayMode = new QComboBox();

	QVBoxLayout* plotLayout = new QVBoxLayout();
	plotLayout->addWidget(displayMode);
	plotLayout->addWidget(plotStatistics);
	plotLayout->addWidget(binTable);
	QWidget* temp = new QWidget();
	temp->setLayout(plotLayout);
	
	QSplitter* plotAndSumary = new QSplitter;
	plotAndSumary->setOrientation(Qt::Horizontal);
	plotAndSumary->addWidget(fileTable);
	plotAndSumary->addWidget(temp);

	btnLoadStatFile = new QPushButton("Load Statistic File");
	QHBoxLayout* buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(btnLoadStatFile, 0, Qt::AlignRight);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(plotAndSumary);
	layout->addLayout(buttonsLayout);
	setLayout(layout);

	connect(btnLoadStatFile, SIGNAL(clicked(bool)), this, SLOT(openStatisticFile()));
	connect(displayMode, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDisplayMode(int)));
	connect(binTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectBar(const QModelIndex&)));
	connect(plotStatistics, SIGNAL(currentSelectedIndexes(std::pair<int, int>)), this, SLOT(selectedIndexesChanged(std::pair<int, int>)));
}

StatisticsTab::~StatisticsTab()
{
}

QSize StatisticsTab::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize StatisticsTab::sizeHint() const
{
	return QSize(640, 480);
}

void StatisticsTab::setProject(Project* project)
{
	currentProject = project;
}

void StatisticsTab::setCurrentItem(Item* item)
{
}

void StatisticsTab::enter()
{
	if(currentProject != 0){
		loadStatisticalData(currentProject->getDirectory().canonicalPath() + QString("/last_song_statistic.tsv"));
	}
}

void StatisticsTab::leave()
{
}

void StatisticsTab::cut()
{
}

void StatisticsTab::copy()
{
	QModelIndexList indexes = fileTable->selectionModel()->selectedIndexes();

	if (indexes.size() > 0) {
		qSort(indexes.begin(), indexes.end()); //sorts by columns
		QString selected_text="";

		QModelIndex previous = indexes.first();
		selected_text.append(fileTable->model()->data(previous).toString());
		indexes.removeFirst();

		foreach (QModelIndex current, indexes) {
			if (current.row() != previous.row()) { 
				selected_text.append('\n'); //new row
			} else {
				selected_text.append('\t'); //new column
			}
			selected_text.append(fileTable->model()->data(current).toString());
			previous = current;
		}
		QApplication::clipboard()->setText(selected_text);
	}
}

void StatisticsTab::paste()
{
}

void StatisticsTab::del()
{
}

void StatisticsTab::toBeDeleted(Item* item)
{
}

void StatisticsTab::toBeClosed(boost::shared_ptr<Project> currentProject)
{
}

void StatisticsTab::openStatisticFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Statistics File"), "../", tr("Statistic Files (*.tsv)"));
	loadStatisticalData(fileName);
}

void StatisticsTab::changeDisplayMode(int index)
{
	drawBarChart(displayMode->currentText().toStdString());
}

// repositions the center of the bar chart to the index selected in the table
void StatisticsTab::selectBar(const QModelIndex& index)
{
	plotStatistics->center(index.column()-1);
}

// receives a std::pair with start and end index of an selected area
void StatisticsTab::selectedIndexesChanged(std::pair<int, int> indexRange)
{
	binTable->selectColumn(indexRange.second+1);
}

// load statistics file and its graphical representation (data)
void StatisticsTab::loadStatisticalData(const QString& fileName)
{
	try{
		loadStatisticsFile(fileName.toStdString());
		displayMode->clear();
		displayMode->addItem(fileName + QString(".ipi"));
		displayMode->addItem(fileName + QString(".train"));
	}catch(std::runtime_error &e){
		QMessageBox::critical(this, QObject::tr("Reading File:"), e.what());
	}
}

// reading a tab seperated file line for line and sending the text to the QPlainTextEdit
void StatisticsTab::loadStatisticsFile(const std::string& fileName)
{
	fileTableModel->clear();
	std::ifstream reportFile(fileName.c_str());
	std::string line;

	try{
		if(reportFile.is_open()){
			while(reportFile.good()){
				getline(reportFile, line);
				QString temp(line.c_str());
				QStringList list = temp.split('\t');

				QList<QStandardItem *> row;
				for(int i = 0; i < list.size(); ++i){
					row.append(new QStandardItem(list[i]));
				}
				fileTableModel->appendRow(row);
			}
			reportFile.close();
			fileTable->resizeColumnsToContents();
		}
	}catch(std::runtime_error &e){
		std::stringstream s;
		s << "Could not read file: \n" << fileName << '\n';
		throw std::runtime_error(s.str());
	}
}

// reads the first line of a file as header, the rest of the lines are values
// the first element in each line is handled as description for the line
// can draw bar charts for any given file list (files must have correct file format) and the directory name
// where these files are located. All values in one file are drawn with the same plot. File format (values are tab seperated): 
// 1. description1	range1 range2 range3 ... (zb [8:10))
// 2. description2 value1 value2 value3 ... (all values are percentages)
// 3. description3 value1 value2 value3 ... (all values are percentages)
void StatisticsTab::loadGraphicalData(const char* fileName, std::vector< std::vector<float> >& data, QStringList& bins, QStringList& names)
{
	std::ifstream file (fileName, std::ios::in);
	try{
		if(file.is_open() && file.good()){
			std::string line;
			if(!file.eof()){
				getline(file, line);
				bins = QString(line.c_str()).split('\t');
				names << bins.takeFirst();
			}
			while(!file.eof()){
				getline(file, line);
				if(line != ""){
					QStringList mean = QString(line.c_str()).split('\t');
					std::vector<float> temp(mean.size()-1);
					if(mean.size() > 0) names << mean[0];
					for(int i = 1; i < mean.size(); ++i){
						temp[i-1] = mean[i].toFloat();
					}
					data.push_back(temp);
				}
			}
		}
		file.close();
	}catch(std::runtime_error &e){
		std::stringstream s;
		s << "Could not read file: \n" << fileName << '\n';
		throw std::runtime_error(s.str());
	}
}

// reads the data, that should be displayed in a bar chart, from a file
// and fills the plot and table in this class accordingly
void StatisticsTab::drawBarChart(const std::string& fileName)
{
	binTableModel->clear();
	plotStatistics->clear();

	std::vector <std::vector<float> > data;
	QStringList bins;
	QStringList names;

	try{
		loadGraphicalData(fileName.c_str() , data, bins, names);
	}catch(std::runtime_error &e){
		QMessageBox::critical(this, QObject::tr("Reading File:"), e.what());
	}
	
	int chart = 0;
	if(data.size() > 0 && bins.size() > 0 && names.size() > 0){

		std::vector<QVector4D> colors(data[0].size());
		for(int i = 0; i < data[0].size(); ++i){
			colors[i] = i%2==0 ? QVector4D(0.52, 0.80, 0.83, 1.0) : QVector4D(0.52, 0.80, 0.92, 1.0);
		}
		for(int i = 1; i < data.size(); i+=2){
			if(data[i-1].size() > 0 && data[i].size() > 0){
				plotStatistics->addBarGraph(data[i-1].begin(), data[i-1].end(), colors.begin(), 1, QVector3D(0, 0, -0.1 * chart), "%");	//TODO: fix grapher near- and far-plane
				plotStatistics->addWhiskerGraph(data[i-1].begin(), data[i-1].end(), data[i].begin(), QVector4D(0, 0, 0, 1), 1, QVector3D(0, 0, -0.1 * chart), "%");	//TODO: fix grapher near- and far-plane
				++chart;
			}
		}

		plotStatistics->center(data[0].size()/2);
		plotStatistics->setNumValues(data[0].size());
	
		QList<QStandardItem *> binKey;
		binKey.append(new QStandardItem(names[0]));
		names.removeFirst();
		for(int i = 0; i < bins.size(); ++i){
			binKey.append(new QStandardItem(bins[i]));
		}
		binTableModel->appendRow(binKey);

		for(int i = 0; i < data.size() && i < names.size(); ++i){
			if(bins.size() == data[i].size()){
				QList<QStandardItem *> binPercentage;
				binPercentage.append(new QStandardItem(names[i]));
				for(int j = 0;  j < bins.size(); ++j){
					binPercentage.append(new QStandardItem(QString::number(data[i].at(j)) + " %"));
				}
				binTableModel->appendRow(binPercentage);
				binPercentage.clear();
			}
		}

		binTable->setFixedHeight(binTable->rowHeight(0)*6);
		binTable->resizeColumnsToContents();
	}
}