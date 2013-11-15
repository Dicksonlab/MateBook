#include <QtGui>

#include <cmath>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <set>
#include "FilesTab.hpp"
#include "ArenaItem.hpp"
#include "RuntimeError.hpp"
#include "makeColorMap.hpp"
#include "GraphicsPrimitives.hpp"
#include "TimeDelegate.hpp"
#include "DateDelegate.hpp"
#include "GenderDelegate.hpp"
#include "FilePathDelegate.hpp"
#include "FilePathNameDelegate.hpp"
#include "ArenasApprovedDelegate.hpp"
#include "TimeOfDayDelegate.hpp"
#include "Project.hpp"
#include "ItemTree.hpp"
#include "splitPath.hpp"
#include "FancyTreeView.hpp"
#include "SongVisitor.hpp"
#include "GroupTree.hpp"
#include "ItemMenu.hpp"
#include "MateBook.hpp"
#include "ConfigDialog.hpp"
#include "../../common/source/Settings.hpp"

FilesTab::FilesTab(MateBook* mateBook, QWidget* parent) : AbstractTab(parent),
	mateBook(mateBook),
	currentProject(NULL)
{
	ItemMenu* cellContextMenu = new ItemMenu();
	filesView = new FancyTreeView(cellContextMenu);
	filesView->setSortingEnabled(true);
	filesView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	filesView->setSelectionBehavior(QAbstractItemView::SelectItems);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;

	QVBoxLayout* buttonsLayout = new QVBoxLayout;
	buttonsLayout->setAlignment(Qt::AlignTop);

	{
		QGroupBox* filesGroupBox = new QGroupBox(tr("Files"));
		filesGroupBox->setFlat(true);
		QVBoxLayout* filesGroupBoxLayout = new QVBoxLayout;
		{
			QPushButton* addButton = new QPushButton("Add...");
			addButton->setToolTip("Add media files to the current project");
			filesGroupBoxLayout->addWidget(addButton);
			connect(addButton, SIGNAL(clicked()), this, SLOT(addFile()));

			QPushButton* changePathButton = new QPushButton("Change Path...");
			changePathButton->setToolTip("Change the part of the path that is common to the selected files");
			filesGroupBoxLayout->addWidget(changePathButton);
			connect(changePathButton, SIGNAL(clicked()), this, SLOT(changeFilePath()));
		}
		filesGroupBox->setLayout(filesGroupBoxLayout);
		buttonsLayout->addWidget(filesGroupBox);
	}

	{
		QGroupBox* videoProcessingGroupBox = new QGroupBox(tr("Video Processing"));
		videoProcessingGroupBox->setFlat(true);
		QGridLayout* videoProcessingGroupBoxLayout = new QGridLayout;
		{
			QPushButton* arenaDetectionButton = new QPushButton("Arena Detection");
			arenaDetectionButton->setToolTip("Detect arenas in the selected videos");
			videoProcessingGroupBoxLayout->addWidget(arenaDetectionButton, 0, 0);
			connect(arenaDetectionButton, SIGNAL(clicked()), this, SLOT(runArenaDetection()));
			QPushButton* resetArenaDetectionButton = new QPushButton(QIcon(":/mb/icons/delete.png"), QString());
			resetArenaDetectionButton->setToolTip("Remove all results");
			videoProcessingGroupBoxLayout->addWidget(resetArenaDetectionButton, 0, 1);
			connect(resetArenaDetectionButton, SIGNAL(clicked()), this, SLOT(resetArenaDetection()));

			QPushButton* flyTrackingButton = new QPushButton("Fly Tracking");
			flyTrackingButton->setToolTip("Track flies in the selected videos and arenas");
			videoProcessingGroupBoxLayout->addWidget(flyTrackingButton, 1, 0);
			connect(flyTrackingButton, SIGNAL(clicked()), this, SLOT(runFlyTracking()));
			QPushButton* resetFlyTrackingButton = new QPushButton(QIcon(":/mb/icons/delete.png"), QString());
			resetFlyTrackingButton->setToolTip("Remove all results except arena information");
			videoProcessingGroupBoxLayout->addWidget(resetFlyTrackingButton, 1, 1);
			connect(resetFlyTrackingButton, SIGNAL(clicked()), this, SLOT(resetFlyTracking()));

			QPushButton* postprocessorButton = new QPushButton("Postprocessor");
			postprocessorButton->setToolTip("Re-run the postprocessor on videos or arenas that have Fly Tracking / Finished status");
			videoProcessingGroupBoxLayout->addWidget(postprocessorButton, 2, 0);
			connect(postprocessorButton, SIGNAL(clicked()), this, SLOT(runPostprocessor()));

			QPushButton* behaviorAnalysisButton = new QPushButton("Behavior Analysis");
			behaviorAnalysisButton->setToolTip("Analyze the behavior for the selected arenas");
			videoProcessingGroupBoxLayout->addWidget(behaviorAnalysisButton, 3, 0);
			connect(behaviorAnalysisButton, SIGNAL(clicked()), this, SLOT(runStatisticalVideoAnalysis()));
			QPushButton* resetBehaviorAnalysisButton = new QPushButton(QIcon(":/mb/icons/delete.png"), QString());
			resetBehaviorAnalysisButton->setToolTip("Remove all results except arena information and tracking data");
			videoProcessingGroupBoxLayout->addWidget(resetBehaviorAnalysisButton, 3, 1);
			connect(resetBehaviorAnalysisButton, SIGNAL(clicked()), this, SLOT(resetStatisticalVideoAnalysis()));
		}
		videoProcessingGroupBox->setLayout(videoProcessingGroupBoxLayout);
		buttonsLayout->addWidget(videoProcessingGroupBox);
	}

	{
		QGroupBox* audioProcessingGroupBox = new QGroupBox(tr("Audio Processing"));
		audioProcessingGroupBox->setFlat(true);
		QGridLayout* audioProcessingGroupBoxLayout = new QGridLayout;
		{
			QPushButton* pulseDetectionButton = new QPushButton("Pulse Detection");
			pulseDetectionButton->setToolTip("Detect pulses in the selected videos");
			audioProcessingGroupBoxLayout->addWidget(pulseDetectionButton, 0, 0);
			connect(pulseDetectionButton, SIGNAL(clicked()), this, SLOT(runPulseDetection()));
			QPushButton* resetPulseDetectionButton = new QPushButton(QIcon(":/mb/icons/delete.png"), QString());
			resetPulseDetectionButton->setToolTip("Remove all results");
			audioProcessingGroupBoxLayout->addWidget(resetPulseDetectionButton, 0, 1);
			connect(resetPulseDetectionButton, SIGNAL(clicked()), this, SLOT(resetPulseDetection()));

			QPushButton* statisticalSongAnalysisButton = new QPushButton("Statistical Song Analysis");
			statisticalSongAnalysisButton->setToolTip("Create statistical summary for song files");
			audioProcessingGroupBoxLayout->addWidget(statisticalSongAnalysisButton, 1, 0);
			connect(statisticalSongAnalysisButton, SIGNAL(clicked()), this, SLOT(runStatisticalSongAnalysis()));
			QPushButton* resetStatisticalSongAnalysisButton = new QPushButton(QIcon(":/mb/icons/delete.png"), QString());
			resetStatisticalSongAnalysisButton->setToolTip("Remove all statistics");
			audioProcessingGroupBoxLayout->addWidget(resetStatisticalSongAnalysisButton, 1, 1);
			connect(resetStatisticalSongAnalysisButton, SIGNAL(clicked()), this, SLOT(resetStatisticalSongAnalysis()));

			QPushButton* toCleanSongButton = new QPushButton("To Clean Song");
			toCleanSongButton->setToolTip("Reload data that was saved by the user");
			audioProcessingGroupBoxLayout->addWidget(toCleanSongButton, 2, 0);
			connect(toCleanSongButton, SIGNAL(clicked()), this, SLOT(reloadCleanData()));
		}
		audioProcessingGroupBox->setLayout(audioProcessingGroupBoxLayout);
		buttonsLayout->addWidget(audioProcessingGroupBox);
	}

	{
		QGroupBox* groupGroupBox = new QGroupBox(tr("Group"));
		groupGroupBox->setFlat(true);
		QVBoxLayout* groupGroupBoxLayout = new QVBoxLayout;
		{
			QPushButton* autoButton = new QPushButton("Auto");
			autoButton->setToolTip("Group arenas automatically using common attributes");
			groupGroupBoxLayout->addWidget(autoButton);
			connect(autoButton, SIGNAL(clicked()), this, SLOT(groupAuto()));

			QPushButton* selectionButton = new QPushButton("Selection");
			selectionButton->setToolTip("Create a group of all the arenas in the current selection");
			groupGroupBoxLayout->addWidget(selectionButton);
			connect(selectionButton, SIGNAL(clicked()), this, SLOT(groupSelection()));
		}
		groupGroupBox->setLayout(groupGroupBoxLayout);
		buttonsLayout->addWidget(groupGroupBox);
	}

	horizontalLayout->addWidget(filesView, 1);

	QScrollArea* buttonsScrollArea = new QScrollArea;
	buttonsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	buttonsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	QWidget* dummyScrollAreaWidget = new QWidget;
	dummyScrollAreaWidget->setLayout(buttonsLayout);
	buttonsScrollArea->setWidget(dummyScrollAreaWidget);
	horizontalLayout->addWidget(buttonsScrollArea);

	setLayout(horizontalLayout);

	// install custom delegates to be used with filesView - you should not share the same instance of a delegate between comboboxes, widget mappers or views!
	filesViewDelegates["Arenas approved"] = new ArenasApprovedDelegate(this);
	filesViewDelegates["Path"] = new FilePathDelegate(this);
	filesViewDelegates["Noise File"] = new FilePathNameDelegate(this);
	filesViewDelegates["Start"] = new TimeDelegate(this);
	filesViewDelegates["End"] = new TimeDelegate(this);
	filesViewDelegates["Date"] = new DateDelegate(this);
	filesViewDelegates["Time of Day"] = new TimeOfDayDelegate(this);
	filesViewDelegates["1st sex"] = new GenderDelegate(this);
	filesViewDelegates["2nd sex"] = new GenderDelegate(this);

	connect(filesView, SIGNAL(columnsWereModified()), this, SLOT(viewWasModified()));
}

FilesTab::~FilesTab()
{
}

QSize FilesTab::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize FilesTab::sizeHint() const
{
	return QSize(640, 480);
}

void FilesTab::readGuiSettings(const QSettings& settings)
{
	lastMediaDirectory = settings.value("lastMediaDirectory", QString()).toString();
}

void FilesTab::writeGuiSettings(QSettings& settings)
{
	settings.setValue("lastMediaDirectory", lastMediaDirectory);
}

void FilesTab::readHeaderSettings()
{
	if (currentProject) {
		filesView->setHeaderVisibility(currentProject->getDirectory().absolutePath() + "/filesTreeView.bin");
	}
}

void FilesTab::writeHeaderSettings()
{
	filesView->saveHeaderVisibility(currentProject->getDirectory().absolutePath() + "/filesTreeView.bin");
}

void FilesTab::setProject(Project* project)
{
	filesView->setModel(project->getItemTree());
	//connect(filesView, SIGNAL(selectionHasChanged()), this, SLOT(updateActionAvailability()));	//TODO: implement and forward to MateBook; can we move this to the constructor after filesView->setModel()?

	// scan the headers to see if there is a custom delegate to use
	for (int columnNumber = 0; columnNumber != filesView->model()->columnCount(); ++columnNumber) {
		QString header = filesView->model()->headerData(columnNumber, Qt::Horizontal).toString();
		std::map<QString, QAbstractItemDelegate*>::const_iterator iter = filesViewDelegates.find(header);
		if (iter != filesViewDelegates.end()) {
			filesView->setItemDelegateForColumn(columnNumber, iter->second);
		}
	}
	currentProject = project;
	filesView->createHeaderContextMenu();
}

void FilesTab::setCurrentItem(Item* item)
{
}

FancyTreeView* FilesTab::getSharedTreeView() const
{
	return filesView;
}

void FilesTab::enter()
{
}

void FilesTab::leave()
{
}

void FilesTab::cut()
{
	filesView->cut();
}

void FilesTab::copy()
{
	filesView->copy();
}

void FilesTab::paste()
{
	filesView->paste();
}

void FilesTab::del()
{
	bool reallyRemove = QMessageBox::warning(this, tr("Remove Videos"), tr("Remove the selected videos from the project?\n\nNOTE: Only the processing results are being deleted, the video file is not."), QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok;
	if (!reallyRemove) {
		return;
	}
	currentProject->removeItems(filesView->getSelectedIndexes());
}

void FilesTab::addFile()
{
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Add Video"), lastMediaDirectory, tr("Video Files (*.avi *.xvid *.mpeg *.mp4 *.mts *.wav);;All Files (*.*)"));
	if (!fileNames.empty()) {
		lastMediaDirectory = fileNames.front();
	}
	QStringList failedToAdd;
	unsigned int numberOfVideosAdded = 0;
	QProgressDialog progress("Adding videos...", "Abort", 0, fileNames.size(), this);
	progress.setWindowModality(Qt::WindowModal);
	foreach (QString fileName, fileNames) {
		if (currentProject->addVideo(fileName)) {
			++numberOfVideosAdded;
		} else {
			failedToAdd << fileName;
		}
		progress.setValue(progress.value() + 1);
		if (progress.wasCanceled()) {
			break;
		}
	}
	progress.reset();	// necessary so it doesn't block when the QMessageBox (see below) is displayed
	if (!failedToAdd.empty()) {
		QMessageBox::warning(this, tr("Adding files"), tr("Could not read files:\n\n%1\n\nThese are either inaccessible or use an unsupported format.").arg(failedToAdd.join("\n")));
	}

	emit showStatusMessage(tr("Added %1 file(s)").arg(numberOfVideosAdded));
}

void FilesTab::changeFilePath() // TODO: into fancyTreeView
{
	QModelIndexList indexes = filesView->getSelectedIndexes();
	// make a map of unique top level items that are selected
	std::map<int, QModelIndex> uniqueRows;
	foreach (QModelIndex index, indexes) {
		if (!index.parent().isValid()) { // top level item
			uniqueRows[index.row()] = index;
		} else { // arena item: add the parent
			uniqueRows[index.parent().row()] = index.parent();
		}
	}

	if (uniqueRows.size() == 0) {
		QMessageBox::warning(this, tr("Changing file path"), tr("Please select one or more files from the list."));
		return;
	}

	QStringList currentFolders;
	QString currentPath = QFileInfo(Item::getItem(uniqueRows.begin()->second)->getFileName()).path();
	QStringList commonFolders = splitPath(currentPath);	// folders that all paths have in common
	
	// compare paths from the beginning and delete the part that is not equal
	for (std::map<int, QModelIndex>::iterator iter = uniqueRows.begin(); iter != uniqueRows.end(); ++iter) {
		currentPath = QFileInfo(Item::getItem(iter->second)->getFileName()).path();
		currentFolders = splitPath(currentPath);

		QStringList::iterator currentIter = currentFolders.begin();
		QStringList::iterator commonIter = commonFolders.begin();
		while (currentIter != currentFolders.end() && commonIter != commonFolders.end()) {
			if (*currentIter != *commonIter) {
				break;
			}
			++currentIter;
			++commonIter;
		}
		commonFolders.erase(commonIter, commonFolders.end());
	}

	if (commonFolders.size() == 0) {
		QMessageBox::warning(this, tr("Changing file path"), tr("The selected paths have no folders in common. Please change them individually."));
		return;
	}

	QString dirName = QFileDialog::getExistingDirectory(this, tr("Select new location of ") + commonFolders.join("/"));

	// create and set new path string for each index
	if (!dirName.isEmpty()) {
		for (std::map<int, QModelIndex>::iterator iter = uniqueRows.begin(); iter != uniqueRows.end(); ++iter) {
			QString oldPath = Item::getItem(iter->second)->getFileName();
			QStringList oldFolders = splitPath(oldPath);
			oldFolders.erase(oldFolders.begin(), oldFolders.begin() + commonFolders.size());
			Item::getItem(iter->second)->setFileName(dirName + "/" + oldFolders.join("/"));
		}
	}
}

void FilesTab::runArenaDetection()
{
	QModelIndexList indexes = filesView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		Item::getItem(index)->runArenaDetection();
	}
}

void FilesTab::runFlyTracking()
{
	QModelIndexList indexes = filesView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		Item::getItem(index)->runFlyTracking();
	}
}

void FilesTab::runPostprocessor()
{
	QModelIndexList indexes = filesView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		Item::getItem(index)->runPostprocessor();
	}
}

void FilesTab::runStatisticalVideoAnalysis()
{
	std::vector<ArenaItem*> selectedAndApproved;
	{
		const std::vector<ArenaItem*> selected = getSelectedArenaItems();
		for (std::vector<ArenaItem*>::const_iterator iter = selected.begin(); iter != selected.end(); ++iter) {
			if ((*iter)->getApproved()) {
				selectedAndApproved.push_back(*iter);
			}
		}
	}

	{	// compile the excel file
		QProgressDialog progressDialog("Performing statistical video analysis...", "Abort", 0, selectedAndApproved.size(), this);
		progressDialog.setWindowModality(Qt::WindowModal);
		QString meanSummaryFilePath = currentProject->getDirectory().filePath("behavior.tsv");
		QFile meanSummaryFile(meanSummaryFilePath);
		if (!meanSummaryFile.open(QFile::WriteOnly | QFile::Truncate)) {
			QMessageBox::warning(this, tr("Statistical video analysis"), tr("Could not open \"%1\" for writing.").arg(meanSummaryFilePath));
		}
		QTextStream meanSummary(&meanSummaryFile);
		bool headersWritten = false;
		for (std::vector<ArenaItem*>::const_iterator iter = selectedAndApproved.begin(); iter != selectedAndApproved.end(); ++iter) {
			bool catSucceeded = true;
			try {
				if (headersWritten) {
					(*iter)->cat("behavior.tsv", meanSummary, true, true, false);
				} else {
					(*iter)->cat("behavior.tsv", meanSummary, true, true, true);
				}
			} catch (RuntimeError&) {
				catSucceeded = false;
			}
			if (catSucceeded) {
				headersWritten = true;
			}
			progressDialog.setValue(progressDialog.value() + 1);
			if (progressDialog.wasCanceled()) {
				return;
			}
		}
		progressDialog.reset();
		QDesktopServices::openUrl(QUrl("file:///" + meanSummaryFilePath));
	}

	{
		// first pass: determine the ethogram sizes
		unsigned int maleMaxWidth = 0;
		unsigned int femaleMaxWidth = 0;
		unsigned int maleTotalHeight = 0;
		unsigned int femaleTotalHeight = 0;
		unsigned int labelMaxWidth = 0;
		QFont font;
		QFontMetrics fontMetrics(font);
		std::vector<QSize> maleEthogramSizes;
		std::vector<QSize> femaleEthogramSizes;
		std::vector<QString> labels;

		{
			QProgressDialog progressDialog("Determining ethogram sizes...", "Abort", 0, selectedAndApproved.size(), this);
			progressDialog.setWindowModality(Qt::WindowModal);
			for (std::vector<ArenaItem*>::const_iterator iter = selectedAndApproved.begin(); iter != selectedAndApproved.end(); ++iter) {
				maleEthogramSizes.push_back((*iter)->getEthogramSize(0));
				femaleEthogramSizes.push_back((*iter)->getEthogramSize(1));
				labels.push_back((*iter)->getFileName() + " / " + QString::fromStdString((*iter)->getId()));

				maleMaxWidth = std::max(maleMaxWidth, maleEthogramSizes.back().width() < 0 ? 0u : static_cast<unsigned int>(maleEthogramSizes.back().width()));
				femaleMaxWidth = std::max(femaleMaxWidth, femaleEthogramSizes.back().width() < 0 ? 0u : static_cast<unsigned int>(femaleEthogramSizes.back().width()));
				maleTotalHeight += maleEthogramSizes.back().height();
				femaleTotalHeight += femaleEthogramSizes.back().height();
				unsigned int thisLabelWidth = std::max(0, fontMetrics.boundingRect(labels.back()).width());
				labelMaxWidth = std::max(labelMaxWidth, thisLabelWidth);

				progressDialog.setValue(progressDialog.value() + 1);
				if (progressDialog.wasCanceled()) {
					return;
				}
			}
			progressDialog.reset();
		}

		// second pass: draw the summary image
		const unsigned int backgroundBrightness = 240;
		const QColor backgroundColor(backgroundBrightness, backgroundBrightness, backgroundBrightness);
		const unsigned int textPadding = 5;
		QImage maleSummary(labelMaxWidth + textPadding + maleMaxWidth, maleTotalHeight, QImage::Format_RGB32);
		QImage femaleSummary(labelMaxWidth + textPadding + femaleMaxWidth, femaleTotalHeight, QImage::Format_RGB32);
		QPainter malePainter(&maleSummary);
		QPainter femalePainter(&femaleSummary);
		malePainter.fillRect(0, 0, maleSummary.width(), maleSummary.height(), backgroundColor);
		femalePainter.fillRect(0, 0, femaleSummary.width(), femaleSummary.height(), backgroundColor);
		unsigned int currentMaleRow = 0;
		unsigned int currentFemaleRow = 0;
		unsigned int currentIndex = 0;

		{
			QProgressDialog progressDialog("Creating summary ethogram...", "Abort", 0, selectedAndApproved.size(), this);
			progressDialog.setWindowModality(Qt::WindowModal);
			for (std::vector<ArenaItem*>::const_iterator iter = selectedAndApproved.begin(); iter != selectedAndApproved.end(); ++iter) {
				QImage maleImage = (*iter)->getEthogram(0);
				QImage femaleImage = (*iter)->getEthogram(1);

				// note that we have a race-condition here: the size could have changed
				// we don't want to have all the images in memory at once, which is why we're making this tradeoff and use a 2-pass algorithm
				if (maleImage.size() != maleEthogramSizes[currentIndex] || femaleImage.size() != femaleEthogramSizes[currentIndex]) {
					QMessageBox::warning(this, tr("Creating summary ethogram"), tr("Ethogram for \"%1\" has changed. Summary histogram creation aborted.").arg(labels[currentIndex]));
					return;
				}

				malePainter.drawImage(labelMaxWidth + textPadding, currentMaleRow, maleImage);
				malePainter.drawText(QRect(0, currentMaleRow, labelMaxWidth, maleImage.height()), Qt::AlignRight, labels[currentIndex]);
				currentMaleRow += maleImage.height();

				femalePainter.drawImage(labelMaxWidth + textPadding, currentFemaleRow, femaleImage);
				femalePainter.drawText(QRect(0, currentFemaleRow, labelMaxWidth, femaleImage.height()), Qt::AlignRight, labels[currentIndex]);
				currentFemaleRow += femaleImage.height();

				progressDialog.setValue(progressDialog.value() + 1);
				if (progressDialog.wasCanceled()) {
					return;
				}

				++currentIndex;
			}
			progressDialog.reset();
		}

		QString maleSummaryPath = currentProject->getDirectory().filePath("0_ethoSummary.png");
		QString femaleSummaryPath = currentProject->getDirectory().filePath("1_ethoSummary.png");

		maleSummary.save(maleSummaryPath);
		femaleSummary.save(femaleSummaryPath);

		QDesktopServices::openUrl(QUrl("file:///" + maleSummaryPath));
		QDesktopServices::openUrl(QUrl("file:///" + femaleSummaryPath));
	}

	{	// heatmaps
		std::vector<QImage> heatmaps = mateBook->getConfigDialog()->getHeatmaps(selectedAndApproved);
		for (size_t i = 0; i != heatmaps.size(); ++i) {
			heatmaps[i].save(currentProject->getDirectory().filePath(QString("heatmap_") + QString::number(i) + ".png"));
		}
	}
}

void FilesTab::runPulseDetection()
{
	QModelIndexList indexes = filesView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		Item::getItem(index)->runPulseDetection();
	}
}

void FilesTab::runStatisticalSongAnalysis()
{
	SongVisitor statisticsVisitor;
	int songFileCount = 0;

	std::vector<FileItem*> selected = getSelectedFileItems();

	QProgressDialog progressDialog("Performing statistical song analysis...", "Abort", 0, selected.size(), this);
	progressDialog.setWindowModality(Qt::WindowModal);

	bool sameOptionId = checkSongOptionIds(selected);
	QString batchName = "";

	if(sameOptionId && selected.size() > 1){
		batchName = QInputDialog::getText(this, tr("Statistical File Name:"), tr("Choose a file name:"), QLineEdit::Normal, QDir::home().dirName(), &sameOptionId);
	}

	if(sameOptionId){
		for(int i = selected.size()-1; i >= 0; --i){
			progressDialog.setValue(progressDialog.value() + 1);
			
			selected[i]->runStatisticalSongAnalysis();
			int prevSongCount = songFileCount;
			songFileCount = statisticsVisitor.visit(selected[i]);
			if (prevSongCount == songFileCount){
				selected.erase(selected.begin()+i);
			}
			if (progressDialog.wasCanceled()) {
				break;
			}
		}

		try{
			if(selected.size() == 1){
				QStringList filePath = splitPath(selected.front()->getFileName());
				std::string fileName = QString("st_" + filePath.last() + "_" + QDateTime::currentDateTime().toString("yyyy-MM-dd-Thh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv").toStdString();
				statisticsVisitor.writeReport(currentProject->getDirectory().canonicalPath().toStdString() + "/" + fileName);
			}else if(selected.size() > 1){
				std::string fileName = QString("stba_" + batchName + "_" + QDateTime::currentDateTime().toString("yyyy-MM-dd-Thh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv").toStdString();
				statisticsVisitor.writeReport(currentProject->getDirectory().canonicalPath().toStdString() + "/" + fileName);
			}
			if(selected.size() > 0){
				statisticsVisitor.writeReport(currentProject->getDirectory().canonicalPath().toStdString() + "/last_song_statistic.tsv");
			}
		}catch(std::runtime_error &e){
			QMessageBox::critical(this, QObject::tr("Writing File:"), e.what());
		}
	}
	
	progressDialog.setValue(selected.size());
}

void FilesTab::resetArenaDetection()
{
	std::vector<FileItem*> selected = getSelectedFileItems();
	for (std::vector<FileItem*>::iterator iter = selected.begin(); iter != selected.end(); ++iter) {
		(*iter)->resetArenaDetection();
	}
}

void FilesTab::resetFlyTracking()
{
	std::vector<Item*> selected = getSelectedItems();
	for (std::vector<Item*>::iterator iter = selected.begin(); iter != selected.end(); ++iter) {
		(*iter)->resetFlyTracking();
	}
}

void FilesTab::resetStatisticalVideoAnalysis()
{
	std::vector<FileItem*> selected = getSelectedFileItems();
	for (std::vector<FileItem*>::iterator iter = selected.begin(); iter != selected.end(); ++iter) {
		(*iter)->resetStatisticalVideoAnalysis();
	}
}

void FilesTab::resetPulseDetection()
{
	std::vector<FileItem*> selected = getSelectedFileItems();
	for (std::vector<FileItem*>::iterator iter = selected.begin(); iter != selected.end(); ++iter) {
		(*iter)->resetPulseDetection();
	}
}

// reload data from raw data files (data right after song analysis)
void FilesTab::resetStatisticalSongAnalysis()
{
	std::vector<FileItem*> selected = getSelectedFileItems();
	for (std::vector<FileItem*>::iterator iter = selected.begin(); iter != selected.end(); ++iter) {
		(*iter)->resetStatisticalSongAnalysis();
	}
}

// reload data from clean data files (data right that was modified and saved by the user)
void FilesTab::reloadCleanData()
{
	std::vector<FileItem*> selected = getSelectedFileItems();
	QProgressDialog progressDialog("Loading Song", "loading", 0, selected.size(), this);
	progressDialog.setCancelButton(0);
	progressDialog.setAutoClose(true);
	for (std::vector<FileItem*>::iterator iter = selected.begin(); iter != selected.end(); ++iter) {
		if((*iter)->getCurrentAudioStage() > Item::AudioRecording){
			try {
				SongResults results = (*iter)->getSongResults();
				results.reloadCleanData();
				(*iter)->updateSongData(results);
				(*iter)->updateStateFromFiles();
			} catch (std::runtime_error& e) {
				QMessageBox::warning(this, tr("Loading clean data"), e.what());
			}
		}
		progressDialog.setValue(progressDialog.value() + 1);
	}
}

void FilesTab::toBeDeleted(FileItem* fileItem)
{
}

std::vector<Item*> FilesTab::getSelectedItems() const
{
	std::vector<Item*> ret;
	std::set<Item*> addedItems;
	QModelIndexList indexes = filesView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		Item* retFileItem = Item::getItem(index);
		if (addedItems.find(retFileItem) == addedItems.end()) {
			ret.push_back(retFileItem);
			addedItems.insert(retFileItem);
		}
	}
	return ret;
}

std::vector<FileItem*> FilesTab::getSelectedFileItems() const
{
	std::vector<FileItem*> ret;
	std::set<FileItem*> addedItems;
	QModelIndexList indexes = filesView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		FileItem* retFileItem = NULL;
		while (!(retFileItem = dynamic_cast<FileItem*>(Item::getItem(index)))) {
			index = index.parent();
			if (!index.isValid()) {
				throw std::logic_error("every Item should be a FileItem or have a FileItem ancestor");
			}
		}
		if (addedItems.find(retFileItem) == addedItems.end()) {
			ret.push_back(retFileItem);
			addedItems.insert(retFileItem);
		}
	}
	return ret;
}

std::vector<ArenaItem*> FilesTab::getSelectedArenaItems() const
{
	std::vector<ArenaItem*> ret;
	std::set<ArenaItem*> addedItems;
	QModelIndexList indexes = filesView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		if (ArenaItem* retArenaItem = dynamic_cast<ArenaItem*>(Item::getItem(index))) {
			if (addedItems.find(retArenaItem) == addedItems.end()) {
				ret.push_back(retArenaItem);
				addedItems.insert(retArenaItem);
			}
		} else if (FileItem* parentItem = dynamic_cast<FileItem*>(Item::getItem(index))) {
			const std::vector<ArenaItem*>& childItems = parentItem->getChildItems();
			for (std::vector<ArenaItem*>::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter) {
				if (addedItems.find(*iter) == addedItems.end()) {
					ret.push_back(*iter);
					addedItems.insert(*iter);
				}
			}
		}
	}
	return ret;
}

Item* FilesTab::getSelectedItem() const
{
	QModelIndex currentIndex = filesView->currentIndex();
	if (!currentIndex.isValid()) {
		return NULL;
	}
	return Item::getItem(currentIndex);
}

FileItem* FilesTab::getSelectedFileItem() const
{
	QModelIndex currentIndex = filesView->currentIndex();

	if (!currentIndex.isValid()) {
		return NULL;
	}

	FileItem* retFileItem = NULL;
	while (!(retFileItem = dynamic_cast<FileItem*>(Item::getItem(currentIndex)))) {
		currentIndex = currentIndex.parent();
		if (!currentIndex.isValid()) {
			throw std::logic_error("every Item should be a FileItem or have a FileItem ancestor");
		}
	}

	return retFileItem;
}

ArenaItem* FilesTab::getSelectedArenaItem() const
{
	QModelIndex currentIndex = filesView->currentIndex();
	if (!currentIndex.isValid() || !currentIndex.parent().isValid()) {	// checks if a child is selected or not
		return NULL;
	}
	return static_cast<ArenaItem*>(Item::getItem(currentIndex));
}

void FilesTab::groupAuto()
{
	//TODO: this was a quick hack; it makes too many assumptions about the itemTree; needs to be rewritten
	currentProject->getGroupTree()->clear();
	for (int parentNumber = 0; parentNumber != filesView->model()->rowCount(); ++parentNumber) {
		QModelIndex parentIndex = filesView->index(parentNumber, 0);
		for (int childNumber = 0; childNumber != currentProject->getItemTree()->rowCount(parentIndex); ++childNumber) {
			QStringList groupNameList;
			for (int columnNumber = 0; columnNumber != filesView->model()->columnCount(); ++columnNumber) {
				if (filesView->isColumnHidden(columnNumber)) {
					continue;
				}
				QModelIndex fieldIndex = currentProject->getItemTree()->index(childNumber, columnNumber, parentIndex);
				QString header = currentProject->getItemTree()->headerData(columnNumber, Qt::Horizontal).toString();
				QString data = currentProject->getItemTree()->data(fieldIndex).toString();
				groupNameList << header + ": " + data;
			}
			QString groupName = groupNameList.join(", ");
			currentProject->getGroupTree()->addArenaItemToGroup((ArenaItem*)(currentProject->getItemTree()->index(childNumber, 0, parentIndex).internalPointer()), groupName);
		}
	}
}

void FilesTab::groupSelection()
{
}

// checks if selected items had different song option settings (song analysis - matlab)
bool FilesTab::checkSongOptionIds(const std::vector<FileItem*>& fileItems)
{
	bool isEqual = true;
	QString text = "Selected song(s):\n\n";
	for(int i = 1; isEqual && i < fileItems.size(); ++i){
		if(fileItems[i]->getSongOptionId() != fileItems[i-1]->getSongOptionId()){
			text += fileItems[i]->getFileName() + "\n";
			isEqual = false;
		}
	}

	if(!isEqual){
		QMessageBox msgBox;
		msgBox.setText(text + "\nwas/were analysed with different\n pulse detection settings or a different noise file was used.");
		msgBox.setInformativeText("Do you wish to proceed?");
		msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		int ret = msgBox.exec();
		switch (ret) {
		   case QMessageBox::Yes:
			   isEqual = true;
			   break;
		   case QMessageBox::Cancel:
			   isEqual = false;
			   break;
		   default:
			   isEqual = false;
			   break;
		}
	}
	return isEqual;
}