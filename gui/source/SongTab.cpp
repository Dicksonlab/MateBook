#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTabWidget>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QInputDialog>
#include <QProgressDialog>
#include <QList>
#include <QString>
#include <QStringList>
#include <QSplitter>
#include <QModelIndex>
#include <QHeaderView>
#include <QScrollArea>
#include <QDir>
#include <cmath>
#include <vector>

#include <iostream>
#include "../../grapher/source/QGLGrapher.hpp"
#include "../../common/source/Settings.hpp"
#include "SongPlayer.hpp"
#include "Video.hpp"
#include "Project.hpp"
#include "SongTab.hpp"
#include "SongStatisticsViewer.hpp"
#include "FileItem.hpp"
#include "ItemTree.hpp"
#include "FancyTreeView.hpp"
#include "TimeDelegate.hpp"
#include "DateDelegate.hpp"
#include "GenderDelegate.hpp"
#include "FilePathDelegate.hpp"
#include "FilePathNameDelegate.hpp"
#include "ArenasApprovedDelegate.hpp"
#include "TimeOfDayDelegate.hpp"
#include "../../common/source/macro.h"
#include "MateBook.hpp"
#include "ConfigDialog.hpp"

SongTab::SongTab(MateBook* mateBook, QWidget* parent) : AbstractTab(parent),
mateBook(mateBook),
currentMode(SongDetails),
currentProject(NULL),
pulseCenterMarkers(NULL),
wasEntered(false)
{
	setFocusPolicy(Qt::StrongFocus); // so that pressed keys are recognized even when the focus is set to the song player
	
	songView = new FancyTreeView;
	songView->setSortingEnabled(true);
	songPlayer = new SongPlayer;

	// Display Settings
	highlightPulsesCheckBox = new QCheckBox("Highlight Pulses");
	highlightPulsesCheckBox->setChecked(true);
	highlightSinesCheckBox = new QCheckBox("Highlight Sines");
	highlightSinesCheckBox->setChecked(true);
	highlightTrainsCheckBox = new QCheckBox("Highlight Trains");
	highlightTrainsCheckBox->setChecked(true);

	QVBoxLayout* settingsLayout = new QVBoxLayout;
	settingsLayout->setAlignment(Qt::AlignTop);
	settingsLayout->addWidget(highlightPulsesCheckBox);
	settingsLayout->addWidget(highlightSinesCheckBox);
	settingsLayout->addWidget(highlightTrainsCheckBox);

	QGroupBox* settingsGroupBox = new QGroupBox("Display Settings");
	settingsGroupBox->setLayout(settingsLayout);
	settingsGroupBox->setFlat(true);

	// Data Options
	btnZoomOut = new QPushButton("Show All");
	btnDeleteAllData = new QPushButton("Delete All Pulses");
	btnDeleteSelectedPulses = new QPushButton("Delete Pulses");
	btnCreatePulse = new QPushButton("Create Pulse");
	btnSongStatistics = new QPushButton("Statistical Analysis");
	btnSaveData = new QPushButton("Save Data");
	btnReloadData = new QPushButton("Reload Data");
	QPushButton* createSineButton = new QPushButton("Create Sine Song Episode");
	QPushButton* deleteSinesButton = new QPushButton("Delete Sine Song Episodes");

	QVBoxLayout* dataoptLayout = new QVBoxLayout;
	dataoptLayout->setAlignment(Qt::AlignTop);
	dataoptLayout->addWidget(btnZoomOut);
	dataoptLayout->addWidget(btnDeleteAllData);
	dataoptLayout->addWidget(btnCreatePulse);
	dataoptLayout->addWidget(btnDeleteSelectedPulses);
	dataoptLayout->addWidget(createSineButton);
	dataoptLayout->addWidget(deleteSinesButton);
	dataoptLayout->addWidget(btnSongStatistics);
	dataoptLayout->addWidget(btnSaveData);
	dataoptLayout->addWidget(btnReloadData);

	QGroupBox* dataoptGroupBox = new QGroupBox("Data Options");
	dataoptGroupBox->setLayout(dataoptLayout);
	dataoptGroupBox->setFlat(true);

	QPushButton* btnNext = new QPushButton("Next Pulse"); 
	btnNext->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
	connect(btnNext, SIGNAL(clicked(bool)), this, SLOT(next()));

	QPushButton* btnPrevious = new QPushButton("Previous Pulse"); 
	btnPrevious->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
	connect(btnPrevious, SIGNAL(clicked(bool)), this, SLOT(previous()));

	QVBoxLayout* songNavLayout = new QVBoxLayout;
	songNavLayout->addWidget(btnNext);
	songNavLayout->addWidget(btnPrevious);

	QGroupBox* songNavGroupBox = new QGroupBox("Song Navigation");
	songNavGroupBox->setLayout(songNavLayout);
	songNavGroupBox->setFlat(true);

	// Song
	QPushButton* btnNextSong = new QPushButton("Next Song");
	btnNextSong->setToolTip("next song");
	btnNextSong->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
	QPushButton* btnPrevSong = new QPushButton("Prev Song");
	btnPrevSong->setToolTip("previous song");
	btnPrevSong->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));

	QVBoxLayout* songoptLayout = new QVBoxLayout;
	songoptLayout->addWidget(btnNextSong);
	songoptLayout->addWidget(btnPrevSong);

	QGroupBox* songoptGroupBox = new QGroupBox("Project Navigation");
	songoptGroupBox->setLayout(songoptLayout);
	songoptGroupBox->setFlat(true);

	QVBoxLayout* optionLayout = new QVBoxLayout;
	optionLayout->setAlignment(Qt::AlignTop);
	optionLayout->addWidget(songNavGroupBox);
	optionLayout->addWidget(songoptGroupBox);
	optionLayout->addWidget(settingsGroupBox);
	optionLayout->addWidget(dataoptGroupBox);

	QScrollArea* buttonsScrollArea = new QScrollArea;
	buttonsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	buttonsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	QWidget* dummyScrollAreaWidget = new QWidget;
	dummyScrollAreaWidget->setLayout(optionLayout);
	buttonsScrollArea->setWidget(dummyScrollAreaWidget);

	QHBoxLayout* playerAndOptionLayout = new QHBoxLayout;
	playerAndOptionLayout->addWidget(songPlayer, 1);
	playerAndOptionLayout->addWidget(buttonsScrollArea);

	QGroupBox* songGroupBox = new QGroupBox();
	songGroupBox->setFlat(true);
	songGroupBox->setLayout(playerAndOptionLayout);

	// pulse details tab
	pulseDetailsTable = new QTableView(this);
	pulseDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	pulseDetailsModel = new QStandardItemModel(this);
	pulseDetailsTable->setModel(pulseDetailsModel);

	ipiDetailsTable = new QTableView(this);
	ipiDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ipiDetailsModel = new QStandardItemModel(this);
	ipiDetailsTable->setModel(ipiDetailsModel);

	trainDetailsTable = new QTableView(this);
	trainDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	trainDetailsModel = new QStandardItemModel(this);
	trainDetailsTable->setModel(trainDetailsModel);

	sineDetailsTable = new QTableView(this);
	sineDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	sineDetailsModel = new QStandardItemModel(this);
	sineDetailsTable->setModel(sineDetailsModel);

	QHBoxLayout* tableLayout = new QHBoxLayout;
	tableLayout->addWidget(pulseDetailsTable);
	tableLayout->addWidget(ipiDetailsTable);
	tableLayout->addWidget(trainDetailsTable);
	tableLayout->addWidget(sineDetailsTable);

	QVBoxLayout* detailsLayout = new QVBoxLayout;
	detailsLayout->addWidget(songView, 0, Qt::AlignTop);
	detailsLayout->addLayout(tableLayout);
	
	QGroupBox* grbSongDetails = new QGroupBox();
	grbSongDetails->setLayout(detailsLayout);

	// statistic tab
	statisticsPlot = new SongStatisticsViewer();
	statisticsPlot->setAntiAliasing(true);
	statisticsPlot->setFontSize(20);

	statisticsTable = new QTableView(this);
	statisticsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	statisticsModel = new QStandardItemModel(this);
	statisticsTable->setModel(statisticsModel);

	QVBoxLayout* plotAndStatLayout = new QVBoxLayout;
	plotAndStatLayout->addWidget(statisticsPlot);
	plotAndStatLayout->addWidget(statisticsTable);

	QHBoxLayout* statLayout = new QHBoxLayout;
	statLayout->addLayout(plotAndStatLayout);
	
	QGroupBox* grbStatisticDetails = new QGroupBox();
	grbStatisticDetails->setLayout(statLayout);

	QTabWidget* detailsTab = new QTabWidget;
	detailsTab->setTabPosition(QTabWidget::North);
	detailsTab->addTab(grbSongDetails, tr("Song Details"));
	detailsTab->addTab(grbStatisticDetails, tr("Statistics"));

	QSplitter* splitter = new QSplitter;
	splitter->setOrientation(Qt::Vertical);
	splitter->addWidget(songGroupBox);
	splitter->addWidget(detailsTab);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(splitter);
	setLayout(layout);

	songViewDelegates["Arenas approved"] = new ArenasApprovedDelegate(this);
	songViewDelegates["Path"] = new FilePathDelegate(this);
	songViewDelegates["Noise File"] = new FilePathNameDelegate(this);
	songViewDelegates["Start"] = new TimeDelegate(this);
	songViewDelegates["End"] = new TimeDelegate(this);
	songViewDelegates["Date"] = new DateDelegate(this);
	songViewDelegates["Time of Day"] = new TimeOfDayDelegate(this);
	songViewDelegates["1st sex"] = new GenderDelegate(this);
	songViewDelegates["2nd sex"] = new GenderDelegate(this);

	connect(highlightPulsesCheckBox, SIGNAL(clicked()), this, SLOT(redrawGraphColors()));
	connect(highlightSinesCheckBox, SIGNAL(clicked()), this, SLOT(redrawGraphColors()));
	connect(highlightTrainsCheckBox, SIGNAL(clicked()), this, SLOT(redrawGraphColors()));
	connect(btnSaveData, SIGNAL(clicked()), this, SLOT(saveData()));
	connect(btnReloadData, SIGNAL(clicked()), this, SLOT(reloadData()));
	connect(btnDeleteSelectedPulses, SIGNAL(clicked()), this, SLOT(deleteSelectedPulses()));
	connect(btnCreatePulse, SIGNAL(clicked()), this, SLOT(createPulse()));
	connect(createSineButton, SIGNAL(clicked()), this, SLOT(createSine()));
	connect(deleteSinesButton, SIGNAL(clicked()), this, SLOT(deleteSelectedSines()));
	connect(btnSongStatistics, SIGNAL(clicked()), this, SLOT(startStatisticalAnalysis()));
	connect(pulseDetailsTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectPulse(const QModelIndex&)));
	connect(trainDetailsTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectPulse(const QModelIndex&)));
	connect(ipiDetailsTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectPulse(const QModelIndex&)));
	connect(sineDetailsTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectSine(const QModelIndex&)));
	connect(songPlayer, SIGNAL(selectionChanged(std::pair<int, int>)), this, SLOT(graphSelectionChanged(std::pair<int, int>)));
	connect(btnNextSong, SIGNAL(clicked()), this, SLOT(loadNextSong()));
	connect(btnPrevSong, SIGNAL(clicked()), this, SLOT(loadPrevSong()));
	connect(btnZoomOut, SIGNAL(clicked()), this, SLOT(zoomOut()));
	connect(btnDeleteAllData, SIGNAL(clicked()), this, SLOT(deleteAllData()));

	connect(songView, SIGNAL(columnsWereModified()), this, SLOT(viewWasModified()));
}

QSize SongTab::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize SongTab::sizeHint() const
{
	return QSize(640, 480);
}

void SongTab::readHeaderSettings()
{
	if (currentProject) {
		songView->setHeaderVisibility(currentProject->getDirectory().absolutePath() + "/songTreeView.bin");
	}
}

void SongTab::writeHeaderSettings()
{
	songView->saveHeaderVisibility(currentProject->getDirectory().absolutePath() + "/songTreeView.bin");
}

void SongTab::setProject(Project* project)
{
	currentProject = project;
	
	//TODO: if we don't jump through hoops here Qt crashes because it wants to access data in the new model using old indexes...file bug report?
	songView->setModel(project->getItemTree());
	songView->createHeaderContextMenu(); //TODO: save settings in project

	// scan the headers to see if there is a custom delegate to use
	for (int columnNumber = 0; columnNumber != songView->model()->columnCount(); ++columnNumber) {
		QString header = songView->model()->headerData(columnNumber, Qt::Horizontal).toString();
		std::map<QString, QAbstractItemDelegate*>::const_iterator iter = songViewDelegates.find(header);
		if (iter != songViewDelegates.end()) {
			songView->setItemDelegateForColumn(columnNumber, iter->second);
		}
	}
	connect(currentProject, SIGNAL(projectWasModified()), this, SLOT(redrawSong()));
	//connect(songView, SIGNAL(selectionHasChanged()), this, SLOT(updateActionAvailability()));	//TODO: implement and forward to MateBook; can we move this to the constructor after filesView->setModel()?
}

// loads the available data of one song into the songPlayer
void SongTab::setCurrentItem(Item* item)
{
	songView->clearSelection();
	songPlayer->clear();
	pulseCenterMarkers = NULL;
	pulseDetailsModel->clear();
	ipiDetailsModel->clear();
	trainDetailsModel->clear();
	sineDetailsModel->clear();
	statisticsModel->clear();
	statisticsPlot->clear();

	FileItem* fileItem = NULL;

	if (!item || !(fileItem = dynamic_cast<FileItem*>(item))) {
		fileItem = NULL;
		return;
	}

	currentItem = fileItem;
	// hide all videos from the songView
	for (int row = 0; row < songView->model()->rowCount(); ++row) {
		// ...except the selected one
		try{
			if (getItem(songView->index(row, 0)) == currentItem) {
				songView->setRowHidden(row, QModelIndex(), false);
				songView->setCurrentIndex(songView->index(row, 0), QItemSelectionModel::SelectCurrent);
			}else{
				songView->setRowHidden(row, QModelIndex(), true);
			}
		}catch(std::runtime_error &e){
			QMessageBox::critical(this, QObject::tr("getItem:"), e.what());
		}
	}
	QModelIndex index = songView->currentIndex();

	try{
		songItem = currentItem->getVideo();
		if(!(currentItem->getCurrentAudioStage() > Item::AudioRecording && currentItem->getCurrentAudioStatus() == Item::Finished)){
			btnSongStatistics->setDisabled(true);
			btnSaveData->setDisabled(true);
			btnReloadData->setDisabled(true);
		}else{
			btnSongStatistics->setDisabled(false);
			btnSaveData->setDisabled(false);
			btnReloadData->setDisabled(false);
		}

		if(!songItem->isVideo()){
			try{
				// set song results path and load data form files
				songResults = currentItem->getSongResults();
				songResults.reloadData();
			}catch(std::runtime_error &e){
				QMessageBox::critical(this, QObject::tr("Reading File:"), e.what());
			}

			if(songItem){
				songPlayer->setCurrentAudio(songItem);
				songPlayer->drawCurrentSong();

				fillStatisticsModel();
				fillDetailsModels();
				redrawGraphColors();
				redrawMarkers();
				songPlayer->centerFrame(0);
			}else{
				QMessageBox::critical(this, tr("Song File:"), "Could not load song");
			}
		}else{
			QMessageBox::critical(this, tr("Wrong format: "), "File is not a .wav file");
		}
	}catch(std::runtime_error &e){
		QMessageBox::critical(this, tr("Reading Song:"), e.what());
	}
}

FancyTreeView* SongTab::getSharedTreeView() const
{
	return songView;
}

void SongTab::enter()
{
	wasEntered = true;
}

void SongTab::leave()
{
	songPlayer->pause();
	wasEntered = false;
}

void SongTab::cut()
{
}

void SongTab::copy()
{
	QModelIndexList indexes = statisticsTable->selectionModel()->selectedIndexes();

	if (indexes.size() > 0) {
		qSort(indexes.begin(), indexes.end()); //sorts by columns
		QString selected_text="";

		QModelIndex previous = indexes.first();
		selected_text.append(statisticsTable->model()->data(previous).toString());
		indexes.removeFirst();

		foreach (QModelIndex current, indexes) {
			if (current.row() != previous.row()) { 
				selected_text.append('\n'); //new row
			} else {
				selected_text.append('\t'); //new column
			}
			selected_text.append(statisticsTable->model()->data(current).toString());
			previous = current;
		}
		QApplication::clipboard()->setText(selected_text);
	}
}

void SongTab::paste()
{
}

void SongTab::del()
{
	deleteSelectedPulses();
	deleteSelectedSines();
}

void SongTab::runPulseDetection()
{
	QModelIndexList indexes = songView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		try{
			getItem(index)->runPulseDetection();
		}catch(std::runtime_error &e){
			QMessageBox::critical(this, QObject::tr("getItem:"), e.what());
		}
	}
}

void SongTab::toBeDeleted(FileItem* currentItem)
{
	//TODO: if a file is deleted or a new project opened, every table and plot has to be cleared
}

// when clicked: jumps to the first next pulse that is not displayed on screen (to the right)
void SongTab::next()
{
	const std::map<size_t, size_t>& pulses = songResults.getPulses();
	if (pulses.size() > 0) {
		size_t nextOutsidePulse = songPlayer->getCurrentSampleNumber() + songPlayer->getValuesDisplayedCount() / 2;
		if (nextOutsidePulse > 0 && nextOutsidePulse < songItem->getSong().size()) {
			std::map<size_t, size_t>::const_iterator it(pulses.upper_bound(nextOutsidePulse));
			songPlayer->focusOn(std::make_pair(it->first, it->second));
		}
	}
}

// when clicked: jumps to the first previous pulse that is not displayed on screen (to the left)
void SongTab::previous()
{
	const std::map<size_t, size_t>& pulses = songResults.getPulses();
	if (pulses.size() > 0) {
		size_t valueOutsideWindow = songPlayer->getCurrentSampleNumber() - songPlayer->getValuesDisplayedCount() / 2;
		if (valueOutsideWindow > 0 && valueOutsideWindow < songItem->getSong().size()) {
			std::map<size_t, size_t>::const_iterator it_current(pulses.lower_bound(songPlayer->getCurrentSampleNumber()));
			std::map<size_t, size_t>::const_iterator it(pulses.lower_bound(valueOutsideWindow));
			if (it == pulses.end() || (it_current == it || (*it).first > valueOutsideWindow)) {
				--it;
			}
			songPlayer->focusOn(std::make_pair(it->first, it->second));
		}
	}
}

// inserts a pair (start + end of current selected area) into pulses, searches for the biggest peak in that area as pulse center and counts the pulseCycles
void SongTab::createPulse()
{
	std::pair<int, int> selection = songPlayer->getSelection();
	if (songResults.createPulse(*songItem, selection)) {
		drawColors(selection);
		songPlayer->changeColor(selection, drawingPad);
		redrawMarkers();

		fillStatisticsModel();
		fillDetailsModels();	// update pulse details table
		emit projectWasModified();
	}
}

// delete all pulses and pulse centers that are in the selected range
void SongTab::deleteSelectedPulses()
{
	std::pair<int, int> selection = songPlayer->getSelection();
	if (songResults.deletePulses(selection)) {
		drawingPad.resize(selection.second - selection.first);
		drawColors(selection);
		songPlayer->changeColor(selection, drawingPad);
		redrawMarkers();

		fillStatisticsModel();
		fillDetailsModels();	// update pulse details table
		emit projectWasModified();
	}
}

void SongTab::createSine()
{
	std::pair<int, int> selection = songPlayer->getSelection();
	if (songResults.createSine(*songItem, selection)) {
		drawingPad.resize(selection.second - selection.first);
		drawColors(selection);
		songPlayer->changeColor(selection, drawingPad);

		fillStatisticsModel();
		fillDetailsModels();	// update pulse details table
		emit projectWasModified();
	}
}

void SongTab::deleteSelectedSines()
{
	std::pair<int, int> selection = songPlayer->getSelection();
	if (songResults.deleteSines(selection)) {
		drawingPad.resize(selection.second - selection.first);
		drawColors(selection);
		songPlayer->changeColor(selection, drawingPad);

		fillStatisticsModel();
		fillDetailsModels();	// update pulse details table
		emit projectWasModified();
	}
}

// saves data to the files the program is working with
void SongTab::saveData()
{
	if(wasEntered){
		if(songItem  && currentItem->getCurrentAudioStage() > Item::AudioRecording){
			if(songResults.getPulseCenters().size() == songResults.getPulses().size()){
				try{
					songResults.saveData();
					songResults.saveCleanData();
					songResults.writeStatisticFile();
					songResults.writeFileCleanedFile();
					currentItem->updateSongData(songResults);
					emit showStatusMessage(tr("Song data saved"));
				}catch(std::runtime_error &e){
					QMessageBox::critical(this, tr("Saving failed: "), e.what());
				}
				emit plotDataChanged();
			}else{
				QMessageBox::critical(this, tr("Saving failed: "), "The number of pulses and pulse centers\ndoes not match");
			}
		}else{
			QMessageBox::critical(this, tr("Song File:"), "No song file is selected");
		}
	}
}

void SongTab::reloadData()
{
	try{
		songResults.reloadData();
		emit showStatusMessage(tr("Song data reloaded"));
	}catch(std::runtime_error &e){
		QMessageBox::critical(this, QObject::tr("Reading File:"), e.what());
	}
	songPlayer->drawCurrentSong(); // otherwise not saved created pulses would stay red
	dataChanged();
}

void SongTab::redrawSong()
{
	if (wasEntered) {
		songPlayer->drawCurrentSong();
		redrawGraphColors();
//		redrawMarkers();
	}
}

void SongTab::startStatisticalAnalysis()
{
	if (!songItem) {
		return;
	}

	QProgressDialog progressDialog("Statistical Song Analysis", "computing", 0, 4, this);
	progressDialog.setAutoClose(true);
	progressDialog.setCancelButton(0);
	progressDialog.setValue(1);
	songResults.calculateStatisticalValues(
		mateBook->getConfigDialog()->getSongAnalysisSettings(),
		songItem->getSampleRate(),
		songItem->getSamples(), 
		currentItem->getStartTime() * songItem->getSampleRate(),
		currentItem->getEndTime() * songItem->getSampleRate()
	);
	progressDialog.setValue(3);
	songPlayer->drawCurrentSong();
	progressDialog.setValue(4);
	dataChanged();
}

// when a row in pulseDetailsTable is selected, the corresponding pulse in songPlayer gets highlighted
void SongTab::selectPulse(const QModelIndex& index)
{
	unsigned int sampleRate = songItem->getSampleRate();
	QVariant data = index.sibling(index.row(), 0).data();

	const std::map<size_t, size_t>& pulses = songResults.getPulses();
	std::map<size_t, size_t>::const_iterator it = pulses.find(data.toUInt());
	if (it != pulses.end()) {
		songPlayer->focusOn(std::make_pair(it->first, it->second));
	}
}

// when a row in sineDetailsTable is selected, the corresponding sine song episode in songPlayer gets highlighted
void SongTab::selectSine(const QModelIndex& index)
{
	unsigned int sampleRate = songItem->getSampleRate();
	QVariant data = index.sibling(index.row(), 0).data();

	const std::map<size_t, size_t>& sines = songResults.getSines();
	std::map<size_t, size_t>::const_iterator it = sines.find(data.toUInt());
	if (it != sines.end()) {
		songPlayer->focusOn(std::make_pair(it->first, it->second));
	}
}

// selects rows of the details tables, if the relevant pulseStart is found
void SongTab::selectTableRow(unsigned int pulseStart)
{
	QList<QStandardItem *> pulseList = pulseDetailsModel->findItems(QString::number(pulseStart));
	QList<QStandardItem *> trainList = trainDetailsModel->findItems(QString::number(pulseStart));
	QList<QStandardItem *> ipiList = ipiDetailsModel->findItems(QString::number(pulseStart));
	if(!pulseList.isEmpty()){
		pulseDetailsTable->selectRow(pulseDetailsModel->indexFromItem(pulseList.front()).row());
	}
	if(!trainList.isEmpty()){
		trainDetailsTable->selectRow(trainDetailsModel->indexFromItem(trainList.front()).row());
	}
	if(!ipiList.isEmpty()){
		ipiDetailsTable->selectRow(ipiDetailsModel->indexFromItem(ipiList.front()).row());
	}
}

// emits signal that next song should be loaded
void SongTab::loadNextSong()
{
	FileItem* item = 0;
	QModelIndex index = songView->currentIndex();
	QModelIndex next = index;
	if(index.isValid()){
		int row = index.row() +1;
		while(item == 0 || item->getSamples() == 0){
			next = index.sibling(row, 0);
			try{
				if(next.isValid()){
					item = dynamic_cast<FileItem*>(currentProject->getItem(next));
					++row;
				}else{
					row = 0;
				}
			}catch(std::runtime_error &e){
				QMessageBox::critical(this, QObject::tr("getItem:"), e.what());
			}
		}
		setCurrentItem(item);
	}
}

// emits signal that previous song should be loaded
void SongTab::loadPrevSong()
{
	FileItem* item = 0;
	QModelIndex index = songView->currentIndex();
	QModelIndex next = index;
	if(index.isValid()){
		int row = index.row() -1;
		while(item == 0 || item->getSamples() == 0){
			next = index.sibling(row, 0);
			try{
				if(next.isValid()){
					item = dynamic_cast<FileItem*>(currentProject->getItem(next));
					--row;
				}else{
					row = songView->model()->rowCount()-1;
				}
			}catch(std::runtime_error &e){
				QMessageBox::critical(this, QObject::tr("getItem:"), e.what());
			}
		}
		setCurrentItem(item);
	}
}

// display whole song at once
void SongTab::zoomOut()
{
	if(songItem == 0){
		return;
	}
	songPlayer->setNumValues(songItem->getSamples());
	songPlayer->currentFrame(songItem->getSamples()/2);
}

void SongTab::deleteAllData()
{
	songResults.clearAllData();
	redrawSong();
	fillStatisticsModel();
	fillDetailsModels();
	emit projectWasModified();
}

void SongTab::graphSelectionChanged(std::pair<int, int> range)
{
	if (!currentItem) {
		return;
	}

	const std::map<size_t, size_t>& pulses = songResults.getPulses();
	const std::map<size_t, size_t>& pulseCenters = songResults.getPulseCenters();

	std::map<size_t, size_t> selectedPulses;

	// be sure to be within song vector range
	range.first = (range.first < 0) ? 0 : range.first;
	range.second = (range.second > songItem->getSong().size()) ? songItem->getSong().size() : range.second;

	for (size_t i = range.first; i < range.second; ++i) {
		std::map<size_t, size_t>::const_iterator it;
		it = pulses.lower_bound(i);
		if (it != pulses.end()) {
			if((*it).first <= range.second && (*it).second <= range.second){
				selectedPulses[(*it).first] = (*it).second;
			}
			i = (*it).first;
		}
	}

	if (selectedPulses.size() > 0) {
		std::map<size_t, size_t>::iterator pulse = selectedPulses.begin();
		selectTableRow((*pulse).first);
	}
}

void SongTab::redrawGraphColors()
{
	std::pair<int, int> selection(0, songItem->getSamples());
	drawingPad.resize(selection.second - selection.first);
	drawColors(selection);
	songPlayer->changeColor(selection, drawingPad);
}

Item* SongTab::getItem(const QModelIndex& index) const
{
	if (!index.isValid()) {
		throw std::runtime_error("invalid index");
	}
//	return static_cast<Item*>(proxyModel->mapToSource(index).internalPointer());	//TODO: remove this if the code below works
	// here we assume that no proxy remaps column 0 and that it contains the address of the Item
	return static_cast<Item*>(index.sibling(index.row(), 0).data(Qt::UserRole).value<void*>());
}

// if pulses/pulse centers got deleted or were inserted
void SongTab::dataChanged()
{
	fillStatisticsModel();
	fillDetailsModels(); // update pulse details table
	redrawGraphColors();
	redrawMarkers();
	emit projectWasModified();
}

// creates a model vor the pulseDetails table view (contains information about detected pulses)
void SongTab::fillDetailsModels()
{
	if (!songItem) {
		return;
	}

	pulseDetailsModel->clear();
	ipiDetailsModel->clear();
	trainDetailsModel->clear();
	sineDetailsModel->clear();

	//====================== Pulse table
	const std::map<size_t, size_t>& pulses = songResults.getPulses();
	const std::map<size_t, size_t>& pulseCenters = songResults.getPulseCenters();
	const std::map<size_t, size_t>& pulseCycles = songResults.getPulseCycles();
	unsigned int sampleRate = songItem->getSampleRate();
	
	if(pulses.size() == pulseCenters.size()){
		pulseDetailsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Pulse Start (sec)")));
		pulseDetailsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Pulse Center (sec)")));
		pulseDetailsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Cycles (#)")));

		// iterate through all known pulses and find the corresponding pulse centers and cpp number
		for(std::map<size_t, size_t>::const_iterator it = pulses.begin(); it != pulses.end(); ++it){
			std::map<size_t, size_t>::const_iterator jt = pulseCenters.find((*it).first);
			std::map<size_t, size_t>::const_iterator kt = pulseCycles.find((*it).first);
			if(jt != pulseCenters.end() && kt != pulseCycles.end()){
				QList<QStandardItem *> row;
				row.append(new QStandardItem(QString::number((*jt).first)));
				row.append(new QStandardItem(QString::number(1.0 / sampleRate * (*jt).second)));

				if(pulses.size() == pulseCycles.size()){
					row.append(new QStandardItem(QString::number((*kt).second)));
				}
				
				pulseDetailsModel->appendRow(row);
			}
		}		
	}else{
		QMessageBox::critical(this, tr("Files not synchronized:"), "The number of pulses and pulse centers\ndoes not match");
	}

	//====================== IPI table
	const std::map<size_t, size_t>& ipis = songResults.getIPI();
	ipiDetailsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("IPI Pulse Start (sec)")));
	ipiDetailsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("IPI Start (sec)")));
	ipiDetailsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("IPI Length (ms)")));

	bool synchronized = true; // if for all ipi start positions a pulse center is found
	for(std::map<size_t, size_t>::const_iterator it = ipis.begin(); synchronized && it != ipis.end(); ++it){
		std::map<size_t, size_t>::const_iterator jt = pulseCenters.find((*it).first);
		if(jt != pulseCenters.end()){ // if pulse center got deleted -> cant find it
			QList<QStandardItem *> row;
			row.append(new QStandardItem(QString::number((*jt).first)));
			row.append(new QStandardItem(QString::number(1.0 / sampleRate * (*jt).second)));
			row.append(new QStandardItem(QString::number(1000.0 / sampleRate * (*it).second )));
			ipiDetailsModel->appendRow(row);
		}else{
			synchronized = false;
		}
	}

	//====================== Train table
	const std::map<size_t, size_t>& trains = songResults.getTrains();
	trainDetailsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Train Pulse Start (sec)")));
	trainDetailsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Train Start (sec)")));
	trainDetailsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Train Length (ms)")));
	trainDetailsModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Nr. of Pulses (#)")));

	for(std::map<size_t, size_t>::const_iterator it = trains.begin(); synchronized && it != trains.end(); ++it){
		std::map<size_t, size_t>::const_iterator jt = pulseCenters.find((*it).first);
		if(jt != pulseCenters.end()){
			QList<QStandardItem *> row;
			row.append(new QStandardItem(QString::number((*jt).first)));
			row.append(new QStandardItem(QString::number(1.0 / sampleRate * (*jt).second)));
			row.append(new QStandardItem(QString::number(1000.0 / sampleRate * ((*it).second - (*it).first))));

			std::map<size_t, size_t>::const_iterator startPulse = pulses.find((*it).first);
			std::map<size_t, size_t>::const_iterator endPulse = pulses.find((*it).second);

			row.append(new QStandardItem(QString::number( std::distance(startPulse, endPulse)+1 )));

			trainDetailsModel->appendRow(row);
		}else{
			synchronized = false;
		}
	}

	//====================== Sine table
	const std::map<size_t, size_t>& sines = songResults.getSines();
	sineDetailsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Sine Song Start (sample)")));
	sineDetailsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Sine Song Start (sec)")));
	sineDetailsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Sine Song Length (ms)")));

	for (std::map<size_t, size_t>::const_iterator it = sines.begin(); it != sines.end(); ++it) {
		QList<QStandardItem*> row;
		row.append(new QStandardItem(QString::number(it->first)));
		row.append(new QStandardItem(QString::number(1.0 / sampleRate * it->first)));
		row.append(new QStandardItem(QString::number(1000.0 / sampleRate * (it->second - it->first))));
		sineDetailsModel->appendRow(row);
	}

	if (!synchronized) { // data files are not up to date
		ipiDetailsModel->clear();
		trainDetailsModel->clear();
		statisticsModel->clear();
		statisticsPlot->clear();
		ipiDetailsModel->insertRow(0, new QStandardItem(QString("Pulse data changed. Run statistical analysis.")));
		trainDetailsModel->insertRow(0, new QStandardItem(QString("Pulse data changed. Run statistical analysis.")));
	} else { // hide pulse start columns
		ipiDetailsTable->hideColumn(0);
		trainDetailsTable->hideColumn(0);
	}
	pulseDetailsTable->hideColumn(0);
	sineDetailsTable->hideColumn(0);

	pulseDetailsTable->resizeColumnsToContents();
	ipiDetailsTable->resizeColumnsToContents();
	trainDetailsTable->resizeColumnsToContents();
	sineDetailsTable->resizeColumnsToContents();
}

void SongTab::fillStatisticsModel()
{
	if(songItem == 0){
		return;
	}
	statisticsModel->clear();

	unsigned int sampleRate = songItem->getSampleRate();
	SongResults::binStruct ipiBin = songResults.getBinData("binIPI");
	SongResults::binStruct cyclesBin = songResults.getBinData("binCycles");
	SongResults::binStruct trainsBin = songResults.getBinData("binTrains");
	//SongResults::binStruct sinusWave = songResults.getBinData("sinusWave");

	size_t modalIPIindex = 0;
	for(int i = 0; i < ipiBin.size(); ++i){
		modalIPIindex = ipiBin[modalIPIindex].second < ipiBin[i].second? i : modalIPIindex;
	}
	size_t modalCycleIndex = 0;
	for(int i = 0; i < cyclesBin.size(); ++i){
		modalCycleIndex = cyclesBin[modalCycleIndex].second < cyclesBin[i].second? i : modalCycleIndex;
	}
	size_t modalTrainIndex = 0;
	for(int i = 0; i < trainsBin.size(); ++i){
		modalTrainIndex = trainsBin[modalTrainIndex].second < trainsBin[i].second? i : modalTrainIndex;
	}

	QList<QStandardItem *> row1;
	row1.append(new QStandardItem(QString("Number of IPIs:")));
	row1.append(new QStandardItem(QString::number(songResults.getIPI().size())));
	row1.append(new QStandardItem(QString("")));
	row1.append(new QStandardItem(QString("Number of pulses:")));
	row1.append(new QStandardItem(QString::number(songResults.getStatisticalValue("PulsesForCycles"))));
	row1.append(new QStandardItem(QString("")));
	row1.append(new QStandardItem(QString("Number of trains:")));
	row1.append(new QStandardItem(QString::number(songResults.getTrains().size())));
	row1.append(new QStandardItem(QString("")));
	row1.append(new QStandardItem(QString("Number of sine song episodes:")));
	row1.append(new QStandardItem(QString::number(songResults.getSines().size())));
	QList<QStandardItem *> row0;
	row0.append(new QStandardItem(QString("IPI Mean (ms):")));
	row0.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("meanIPI"))));
	row0.append(new QStandardItem(QString("")));
	row0.append(new QStandardItem(QString("Cycles Mean (#):")));
	row0.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("meanCPP"))));
	row0.append(new QStandardItem(QString("")));
	row0.append(new QStandardItem(QString("Trains Mean (pulses):")));
	row0.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("meanTrain"))));
	row0.append(new QStandardItem(QString("")));
	row0.append(new QStandardItem(QString("Sine Song Episode Duration Mean (ms):")));
	row0.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("meanSineDuration"))));
	QList<QStandardItem *> row2;
	row2.append(new QStandardItem(QString("IPI Standard deviation (ms):")));
	row2.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("standardDeviationIPI"))));
	row2.append(new QStandardItem(QString("")));
	row2.append(new QStandardItem(QString("Cycles Standard deviation (#):")));
	row2.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("standardDeviationCycle"))));
	row2.append(new QStandardItem(QString("")));
	row2.append(new QStandardItem(QString("Trains Standard deviation (pulses):")));
	row2.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("standardDeviationTrain"))));
	row2.append(new QStandardItem(QString("")));
	row2.append(new QStandardItem(QString("Total Sine Song Duration (ms):")));
	row2.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("totalSineDuration"))));
	QList<QStandardItem *> row3;
	row3.append(new QStandardItem(QString("IPI Standard error (ms):")));
	row3.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("standardErrorIPI"))));
	row3.append(new QStandardItem(QString("")));
	row3.append(new QStandardItem(QString("Cycles Standard error (#):")));
	row3.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("standardErrorCycle"))));
	row3.append(new QStandardItem(QString("")));
	row3.append(new QStandardItem(QString("Trains Standard error (pulses):")));
	row3.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("standardErrorTrain"))));
	QList<QStandardItem *> row4;
	row4.append(new QStandardItem(QString("Modal IPI (ms)")));
	if(ipiBin.size() > 0){
		row4.append(new QStandardItem(QString::number(ipiBin[modalIPIindex].first.first) + " - " + QString::number(ipiBin[modalIPIindex].first.second)));
	}else{
		row4.append(new QStandardItem(QString("N.A.")));
	}
	row4.append(new QStandardItem(QString("")));
	row4.append(new QStandardItem(QString("Modal Cycle (#)")));
	if(cyclesBin.size() > 0){
		row4.append(new QStandardItem(QString::number(cyclesBin[modalCycleIndex].first.second)));
	}else{
		row4.append(new QStandardItem(QString("N.A.")));
	}
	row4.append(new QStandardItem(QString("")));
	row4.append(new QStandardItem(QString("Modal Train (pulses)")));
	if(trainsBin.size() > 0){
		row4.append(new QStandardItem(QString::number(trainsBin[modalTrainIndex].first.first) + " - " + QString::number(trainsBin[modalTrainIndex].first.second)));
	}else{
		row4.append(new QStandardItem(QString("N.A.")));
	}
	QList<QStandardItem *> row5;
	row5.append(new QStandardItem(QString("")));
	row5.append(new QStandardItem(QString("")));
	row5.append(new QStandardItem(QString("")));
	row5.append(new QStandardItem(QString("Pulses/min (#):")));
	row5.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("pulsesPerMinute"))));
	QList<QStandardItem *> row6;
	row6.append(new QStandardItem(QString("IPI Quantiles (ms):")));
	row6.append(new QStandardItem(QString("")));
	row6.append(new QStandardItem(QString("")));
	row6.append(new QStandardItem(QString("Pulses/min excluding pulses (#):")));
	row6.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("pulsesPerMinuteExclude"))));
	QList<QStandardItem *> row7;
	row7.append(new QStandardItem(QString("Q25 (ms):")));
	row7.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("quartil25IPI"))));
	QList<QStandardItem *> row8;
	row8.append(new QStandardItem(QString("Q50 (ms):")));
	row8.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("quartil50IPI"))));
	QList<QStandardItem *> row9;
	row9.append(new QStandardItem(QString("Q75 (ms):")));
	row9.append(new QStandardItem(isValidValue(songResults.getStatisticalValue("quartil75IPI"))));

	statisticsModel->appendRow(row1);
	statisticsModel->appendRow(row0);
	statisticsModel->appendRow(row2);
	statisticsModel->appendRow(row3);
	statisticsModel->appendRow(row4);
	statisticsModel->appendRow(row5);
	statisticsModel->appendRow(row6);
	statisticsModel->appendRow(row7);
	statisticsModel->appendRow(row8);
	statisticsModel->appendRow(row9);

	statisticsTable->resizeColumnsToContents();

	std::vector <SongResults::binStruct> graphData(3);
	graphData[0] = ipiBin;
	graphData[1] = cyclesBin;
	graphData[2] = trainsBin;
	//graphData[3] = sinusWave;

	QStringList names = (QStringList() << "IPI Graph" << "Cycles Graph" << "Trains Graph");

	statisticsPlot->setGraph(graphData, names);
}

QString SongTab::isValidValue(const float number)
{
	QString value = "";
	if(number > -1){
		value = QString::number(number);
	}else{
		value = "N.A.";
	}
	return value;
}

void SongTab::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_P && (e->modifiers() & Qt::ShiftModifier)) {
		deleteSelectedPulses();
	} else if (e->key() == Qt::Key_P) {
		createPulse();
	} else if (e->key() == Qt::Key_S && (e->modifiers() & Qt::ShiftModifier)) {
		deleteSelectedSines();
	} else if (e->key() == Qt::Key_S) {
		createSine();
	} else if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Right) {
		next();
	} else if (e->key() == Qt::Key_Left) {
		previous();
	} else {
		AbstractTab::keyPressEvent(e);
	}
}

void SongTab::drawColors(std::pair<int, int> indexRange)
{
	drawingPad.resize(indexRange.second - indexRange.first);
	std::fill(drawingPad.begin(), drawingPad.end(), QVector4D(0.0, 0.0, 0.0, 1.0));

	//TODO: are pulse, sine and train ranges [begin,end)? Then fix createPulse and remove the -1 here.
	size_t startSample = indexRange.first;
	size_t endSample = indexRange.second + 1;

	//TODO: much more efficient than going through the whole thing would be to search the map with lower_bound
	if (highlightTrainsCheckBox->isChecked()) {
		const std::map<size_t, size_t>& trains = songResults.getTrains();
		for (std::map<size_t, size_t>::const_iterator it = trains.begin(); it != trains.end(); ++it) {
			if (it->first >= startSample && it->first < endSample && it->second >= startSample && it->second < endSample) {
				std::fill(drawingPad.begin() + (it->first - startSample), drawingPad.begin() + (it->second - startSample), QVector4D(0.0, 1.0, 0.0, 1.0));
			}
		}
	}

	//TODO: much more efficient than going through the whole thing would be to search the map with lower_bound
	if (highlightSinesCheckBox->isChecked()) {
		const std::map<size_t, size_t>& sines = songResults.getSines();
		for (std::map<size_t, size_t>::const_iterator it = sines.begin(); it != sines.end(); ++it) {
			if (it->first >= startSample && it->first < endSample && it->second >= startSample && it->second < endSample) {
				std::fill(drawingPad.begin() + (it->first - startSample), drawingPad.begin() + (it->second - startSample), QVector4D(0.0, 0.0, 1.0, 1.0));
			}
		}
	}

	//TODO: much more efficient than going through the whole thing would be to search the map with lower_bound
	if (highlightPulsesCheckBox->isChecked()) {
		const std::map<size_t, size_t>& pulses = songResults.getPulses();
		for (std::map<size_t, size_t>::const_iterator it = pulses.begin(); it != pulses.end(); ++it) {
			if (it->first >= startSample && it->first < endSample && it->second >= startSample && it->second < endSample) {
				std::fill(drawingPad.begin() + (it->first - startSample), drawingPad.begin() + (it->second - startSample), QVector4D(1.0, 0.0, 0.0, 1.0));
			}
		}
	}
}

void SongTab::redrawMarkers()
{
	if (!songItem) {
		return;
	}

	if (pulseCenterMarkers) {
		songPlayer->removeMarker(pulseCenterMarkers);
		pulseCenterMarkers = NULL;
	}

	const std::map<size_t, size_t>& pulseCenters = songResults.getPulseCenters();
	if (!pulseCenters.empty()) {
		markerXTemp.resize(pulseCenters.size());
		markerYTemp.resize(pulseCenters.size());

		const std::vector<float>& currentSong = songItem->getSong();
		std::map<size_t, size_t>::const_iterator iter = pulseCenters.begin();
		for (size_t i = 0; i != pulseCenters.size(); ++i) {
			markerXTemp[i] = iter->second;
			markerYTemp[i] = currentSong[iter->second];
			++iter;
		}

		std::string filename(":/mb/icons/marker.png");
		pulseCenterMarkers = songPlayer->addMarker(filename, &markerXTemp[0], &markerYTemp[0], pulseCenters.size(), QVector3D(0, 0, -0.2), 0.5, 45);
	}
}
