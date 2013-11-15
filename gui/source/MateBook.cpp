#include <iostream>
#include <math.h>
#include <stdexcept>
#include <iomanip>
#include <QtGui>
#include <QApplication>
#include <boost/bind.hpp>
#include "MateBook.hpp"
#include "ItemTree.hpp"
#include "FileItem.hpp"
#include "ArenaItem.hpp"
#include "VideoRenderer.hpp"
#include "VideoPlayer.hpp"
#include "SongTab.hpp"
#include "JobQueue.hpp"
#include "../../common/source/Singleton.hpp"
#include "global.hpp"
#include "Project.hpp"
#include "FilesTab.hpp"
#include "VideoTab.hpp"
#include "ArenaTab.hpp"
#include "GroupsTab.hpp"
#include "GraphicsPrimitives.hpp"
#include "TimeDelegate.hpp"
#include "DateDelegate.hpp"
#include "GenderDelegate.hpp"
#include "FilePathDelegate.hpp"
#include "FilePathNameDelegate.hpp"
#include "ArenasApprovedDelegate.hpp"
#include "Version.hpp"
#include "SongResults.hpp"
#include "splitPath.hpp"
#include "StatisticsTab.hpp"
#include "FancyTreeView.hpp"
#include "ConfigDialog.hpp"
#include "../../common/source/debug.hpp"

MateBook::MateBook(QWidget* parent, Qt::WFlags flags) : QMainWindow(parent, flags),
	currentProject(),
	currentMode(FilesMode)
{
	setUnifiedTitleAndToolBarOnMac(true);
	setAcceptDrops(true);

	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createStyles();
	createTabs();

	configDialog = new ConfigDialog(trackerSettings, this);
	configDialog->resize(700, 550);
	readGuiSettings();

	newProjectAction->trigger();
	
	//TODO: deal with program arguments
	//if (Singleton<Settings>::instance().defined("project")) {
	//	openProject(QString::fromStdString(Singleton<Settings>::instance().get("project").as<std::string>()));
	//}

	//qApp->installEventFilter(this);
}

MateBook::~MateBook()
{
}

boost::shared_ptr<Project> MateBook::getCurrentProject() const
{
	return currentProject;
}

void MateBook::closeEvent(QCloseEvent* event)
{
	modeTab->setCurrentIndex(0);	// switch back to Files tab so the current tab gets a leave() notification
	if (maybeSave()) {
		writeGuiSettings();
		event->accept();
	} else {
		event->ignore();
		return;
	}
	emit closeMateBook();
}

void MateBook::newProject()
{
	if (maybeSave()) {
		// hold the old project in memory until the end of this method
		// this way its data can still be accessed during the switch
		boost::shared_ptr<Project> previousProject = currentProject;

		try {
			currentProject = boost::shared_ptr<Project>(new Project(this));
		} catch (std::runtime_error& e) {
			if (!currentProject) {
				// this is bad: having a currentProject is a class invariant,
				// but we don't have one and we just failed to create a new one, so we have to exit
				QMessageBox::critical(this, tr("Creating a new project"), e.what());
				exit(1);
			}
			QMessageBox::warning(this, tr("Creating a new project"), e.what());
			return;
		}

		for (int tabIndex = 0; tabIndex != modeTab->count(); ++tabIndex) {
			assert_cast<AbstractTab*>(modeTab->widget(tabIndex))->setProject(currentProject.get());
		}
		
		//TODO: we're not loading the event setting defaults here...bug or feature?
		statusBar()->showMessage(tr("New project"), statusDisplayDuration);
		setWindowModified(false);
		setWindowTitle(tr("%1[*] - %2").arg(tr("New Project")).arg(tr("MateBook")));

		connect(currentProject.get(), SIGNAL(projectWasModified()), this, SLOT(projectWasModified()));
	}
}

void MateBook::openProject()
{
	if (maybeSave()) {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open Project"), lastProjectDirectory, tr("MateBook Project Files (*.mbp)"));
		if (fileName.isEmpty()) {
			return;
		}
		openProject(fileName);
	}
}

void MateBook::openProject(const QString& fileName)
{
	lastProjectDirectory = fileName;

	// hold the old project in memory until the end of this method
	// this way its data can still be accessed during the switch
	boost::shared_ptr<Project> previousProject = currentProject;

	try {
		currentProject = boost::shared_ptr<Project>(new Project(this, fileName));
	} catch (std::runtime_error& e) {
		QMessageBox::warning(this, tr("Opening project"), e.what());
		return;
	}

	for (int tabIndex = 0; tabIndex != modeTab->count(); ++tabIndex) {
		assert_cast<AbstractTab*>(modeTab->widget(tabIndex))->setProject(currentProject.get());
	}

	readTrackerSettings(currentProject->getDirectory().filePath("tracker.tsv"));
	readHeaderSettings();

	statusBar()->showMessage(tr("Project %1 opened").arg(fileName), statusDisplayDuration);
	setWindowModified(false);
	setWindowTitle(tr("%1[*] - %2").arg(currentProject->getFileName()).arg(tr("MateBook")));

	connect(currentProject.get(), SIGNAL(projectWasModified()), this, SLOT(projectWasModified()));
}

bool MateBook::saveProject()
{
	if (currentProject->isUnnamed()) {
		return saveProjectAs();
	}

	try {
		currentProject->save();
		writeTrackerSettings(currentProject->getDirectory().filePath("tracker.tsv"));
		writeHeaderSettings();
		writeMatlabImportScript();
		emit savedProject();	//TODO: why is this a signal?
	} catch (std::runtime_error& e) {
		QMessageBox::warning(this, tr("Saving project"), e.what());
		return false;
	}
	statusBar()->showMessage(tr("Project saved"), statusDisplayDuration);
	setWindowModified(false);
	return true;
}

bool MateBook::saveProjectAs()
{
	if (Singleton<JobQueue>::instance().queuedJobsCount() + Singleton<JobQueue>::instance().runningJobsCount()) {
		bool abortJobs = QMessageBox::question(this, tr("Abort Jobs"), tr("Cannot save the project under a different name while data is being processed. Do you want to abort all jobs and save?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes;
		if (!abortJobs) {
			return false;
		}
		Singleton<JobQueue>::instance().abortAllJobs();
	}

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Project"), lastProjectDirectory, tr("MateBook Project Files (*.mbp)"));
	if (fileName.isEmpty()) {
		return false;
	}
	lastProjectDirectory = fileName;

	try {
		currentProject->saveAs(fileName);
		writeTrackerSettings(currentProject->getDirectory().filePath("tracker.tsv"));
		writeHeaderSettings();
		writeMatlabImportScript();
		emit savedProject();	//TODO: why is this a signal?
	} catch (std::runtime_error& e) {
		QMessageBox::warning(this, tr("Saving project"), e.what());
		return false;
	}

	saveAsWasSuccessful();
	return true;
}

void MateBook::saveAsWasSuccessful()
{
	statusBar()->showMessage(tr("Project saved as %1").arg(currentProject->getFileName()), statusDisplayDuration);
	setWindowModified(false);
	setWindowTitle(tr("%1[*] - %2").arg(currentProject->getFileName()).arg(tr("MateBook")));
}

void MateBook::readTrackerSettings(const QString& fileName)
{
	trackerSettings.importFrom(fileName.toStdString());
}

void MateBook::writeTrackerSettings(const QString& fileName) const
{
	trackerSettings.exportTo(fileName.toStdString());
}

ConfigDialog* MateBook::getConfigDialog()
{
	return configDialog;
}

void MateBook::showConfigDialog()
{
	configDialog->exec();
}

void MateBook::about()
{
	QMessageBox::about(this, tr("About MateBook"),
		QString("<table>") +
		"<tr><td>" + tr("Current version: ") + "</td><td>" + QString::number(Version::current) + "</td></tr>" +
		"<tr><td>" + tr("Earliest resettable project: ") + "</td><td>" + QString::number(Version::earliestResettableProject) + "</td></tr>" +
		"<tr><td>" + tr("Earliest convertible project: ") + "</td><td>" + QString::number(Version::earliestConvertibleProject) + "</td></tr>" +
		QString("</table>")
	);
}

void MateBook::projectWasModified()
{
	setWindowModified(true);
}

void MateBook::changeMode(int tabIndex)
{
	Mode newMode = static_cast<Mode>(tabIndex);
	if (newMode == currentMode) {
		return;
	}

	assert_cast<AbstractTab*>(modeTab->widget(newMode))->setCurrentItem(assert_cast<FilesTab*>(modeTab->widget(0))->getSelectedItem());
	assert_cast<AbstractTab*>(modeTab->widget(newMode))->enter();

	assert_cast<AbstractTab*>(modeTab->widget(currentMode))->leave();

	currentMode = newMode;
}

void MateBook::cut()
{
	assert_cast<AbstractTab*>(modeTab->widget(currentMode))->cut();
}

void MateBook::copy()
{
	assert_cast<AbstractTab*>(modeTab->widget(currentMode))->copy();
}

void MateBook::paste()
{
	assert_cast<AbstractTab*>(modeTab->widget(currentMode))->paste();
}

void MateBook::del()
{
	assert_cast<AbstractTab*>(modeTab->widget(currentMode))->del();
}

void MateBook::updateSelectedItemState()
{
	if (FileItem* selected = assert_cast<FilesTab*>(modeTab->widget(0))->getSelectedFileItem()) {
		selected->updateStateFromFiles();
	}
}

void MateBook::addRowToFormLayout(QFormLayout* layout, const QString& key, const QString& labelText, double defaultValue, double minValue, double maxValue, const QString& unit, const QString& labelToolTip, const QString& fieldToolTip)
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
	trackerSettings.add<double>(key.toStdString(), boost::bind(&QDoubleSpinBox::value, spinBox), boost::bind(&QDoubleSpinBox::setValue, spinBox, _1));
}

void MateBook::setCheckedId(QButtonGroup* buttonGroup, int id)
{
	if (QAbstractButton* button = buttonGroup->button(id)) {
		button->setChecked(true);
	}
}

void MateBook::showHelpBrowser()
{
	QDesktopServices::openUrl(QUrl("file:///" + QFileInfo("../qt/doc/pages/index.html").absoluteFilePath()));
}

void MateBook::bugReport()
{
	QDesktopServices::openUrl(QUrl("http://ross/bugs/"));
}

void MateBook::setStatusMessage(const QString& text)
{
	statusBar()->showMessage(text, statusDisplayDuration);
}

void MateBook::moreDetail()
{
	// find the index of the visualizer that has requested more detail and change to the tab that's to the right of that one
	QObject* signalSender = sender();
	for (int tabIndex = 0; tabIndex + 1 < modeTab->count(); ++tabIndex) {	// +1 because we cannot switch to the right for the last widget
		if (signalSender == modeTab->widget(tabIndex)) {
			modeTab->setCurrentIndex(tabIndex + 1);
		}
	}
}

void MateBook::createActions()
{
	newProjectAction = new QAction(QIcon(":/mb/icons/new.png"), tr("&New Project"), this);
	newProjectAction->setShortcuts(QKeySequence::New);
	newProjectAction->setStatusTip(tr("Create a new project"));
	connect(newProjectAction, SIGNAL(triggered()), this, SLOT(newProject()));

	openProjectAction = new QAction(QIcon(":/mb/icons/open.png"), tr("&Open Project..."), this);
	openProjectAction->setShortcuts(QKeySequence::Open);
	openProjectAction->setStatusTip(tr("Open an existing project"));
	connect(openProjectAction, SIGNAL(triggered()), this, SLOT(openProject()));

	saveProjectAction = new QAction(QIcon(":/mb/icons/save.png"), tr("&Save Project"), this);
	saveProjectAction->setShortcuts(QKeySequence::Save);
	saveProjectAction->setStatusTip(tr("Save the project to disk"));
	connect(saveProjectAction, SIGNAL(triggered()), this, SLOT(saveProject()));

	saveProjectAsAction = new QAction(tr("Save Project &As..."), this);
	saveProjectAsAction->setShortcuts(QKeySequence::SaveAs);
	saveProjectAsAction->setStatusTip(tr("Save the project under a new name"));
	connect(saveProjectAsAction, SIGNAL(triggered()), this, SLOT(saveProjectAs()));

	exitAction = new QAction(tr("E&xit"), this);
	exitAction->setShortcut(tr("Ctrl+Q"));
	exitAction->setStatusTip(tr("Exit the application"));
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	cutAction = new QAction(QIcon(":/mb/icons/cut.png"), tr("Cu&t"), this);
	cutAction->setShortcuts(QKeySequence::Cut);
	cutAction->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
	connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));

	copyAction = new QAction(QIcon(":/mb/icons/copy.png"), tr("&Copy"), this);
	copyAction->setShortcuts(QKeySequence::Copy);
	copyAction->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

	pasteAction = new QAction(QIcon(":/mb/icons/paste.png"), tr("&Paste"), this);
	pasteAction->setShortcuts(QKeySequence::Paste);
	pasteAction->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
	connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

	delAction = new QAction(QIcon(":/mb/icons/delete.png"), tr("&Delete"), this);
	delAction->setShortcut(tr("Delete"));
	delAction->setStatusTip(tr("Remove the current selection's content"));
	connect(delAction, SIGNAL(triggered()), this, SLOT(del()));

	showConfigDialogAction = new QAction(tr("&Settings..."), this);
	showConfigDialogAction->setStatusTip(tr("Open the Settings dialog window"));
	connect(showConfigDialogAction, SIGNAL(triggered()), this, SLOT(showConfigDialog()));

	aboutAction = new QAction(tr("&About"), this);
	aboutAction->setStatusTip(tr("Show the application's About box"));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	showHelpBrowserAction = new QAction(tr("Help Contents"), this);
    showHelpBrowserAction->setShortcut(QKeySequence::HelpContents);
	connect(showHelpBrowserAction, SIGNAL(triggered()), this, SLOT(showHelpBrowser()));

	bugReportAction = new QAction(tr("Bug Reports"), this);
	connect(bugReportAction, SIGNAL(triggered()), this, SLOT(bugReport()));
}

void MateBook::createMenus()
{
	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(newProjectAction);
	fileMenu->addAction(openProjectAction);
	fileMenu->addAction(saveProjectAction);
	fileMenu->addAction(saveProjectAsAction);
	fileMenu->addSeparator();
	fileMenu->addAction(showConfigDialogAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu(tr("&Edit"));
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(pasteAction);
	editMenu->addAction(delAction);

	menuBar()->addSeparator();

	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAction);
	helpMenu->addAction(showHelpBrowserAction);
	helpMenu->addAction(bugReportAction);
}

void MateBook::createToolBars()
{
	fileToolBar = addToolBar(tr("File"));
	fileToolBar->addAction(newProjectAction);
	fileToolBar->addAction(openProjectAction);
	fileToolBar->addAction(saveProjectAction);

	editToolBar = addToolBar(tr("Edit"));
	editToolBar->addAction(cutAction);
	editToolBar->addAction(copyAction);
	editToolBar->addAction(pasteAction);
	editToolBar->addAction(delAction);
}

void MateBook::createStatusBar()
{
	statusBar()->showMessage(tr("Ready"));
}

void MateBook::createStyles()
{
	QString stylesheet = "";
		stylesheet += "QTreeView::item:selected:active{selection-color: black; background-color: rgb(198, 226, 255); color: black;}"; // active window
		stylesheet += "QTreeView::item:selected:!active{selection-color: black; background-color: rgb(211, 211, 211); color: black;}"; // inactive window
    setStyleSheet(stylesheet);
}

void MateBook::createTabs()
{
	modeTab = new QTabWidget;
	modeTab->setTabPosition(QTabWidget::South);
	setCentralWidget(modeTab);
	connect(modeTab, SIGNAL(currentChanged(int)), this, SLOT(changeMode(int)));

	SongTab* songTab = new SongTab(this);
	FilesTab* filesTab = new FilesTab(this);
	VideoTab* videoTab = new VideoTab;

	modeTab->addTab(filesTab, tr("Files"));
	modeTab->addTab(videoTab, tr("Video"));
	modeTab->addTab(new ArenaTab, tr("Arena"));
	modeTab->addTab(new GroupsTab, tr("Groups"));
	modeTab->addTab(songTab, tr("Song"));
	
	connect(filesTab, SIGNAL(showStatusMessage(const QString&)), this, SLOT(setStatusMessage(const QString&)));
	connect(videoTab, SIGNAL(moreDetail()), this, SLOT(moreDetail()));
	connect(songTab, SIGNAL(showStatusMessage(const QString&)), this, SLOT(setStatusMessage(const QString&)));
	connect(songTab, SIGNAL(plotDataChanged()), this, SLOT(updateSelectedItemState()));

	modeTab->addTab(new StatisticsTab, tr("Statistics"));

	FancyTreeView* filesView = assert_cast<FilesTab*>(modeTab->widget(0))->getSharedTreeView();
	FancyTreeView* videoView = assert_cast<VideoTab*>(modeTab->widget(1))->getSharedTreeView();
	FancyTreeView* songView = assert_cast<SongTab*>(modeTab->widget(4))->getSharedTreeView();

	connect(filesView, SIGNAL(selectionHasChanged(const QItemSelection&, const QItemSelection&)), videoView, SLOT(changeSelection(const QItemSelection&, const QItemSelection&)));
	connect(filesView, SIGNAL(selectionHasChanged(const QItemSelection&, const QItemSelection&)), songView, SLOT(changeSelection(const QItemSelection&, const QItemSelection&)));
	connect(videoView, SIGNAL(selectionHasChanged(const QItemSelection&, const QItemSelection&)), filesView, SLOT(changeSelection(const QItemSelection&, const QItemSelection&)));
	connect(videoView, SIGNAL(selectionHasChanged(const QItemSelection&, const QItemSelection&)), songView, SLOT(changeSelection(const QItemSelection&, const QItemSelection&)));
	connect(songView, SIGNAL(selectionHasChanged(const QItemSelection&, const QItemSelection&)), filesView, SLOT(changeSelection(const QItemSelection&, const QItemSelection&)));
	connect(songView, SIGNAL(selectionHasChanged(const QItemSelection&, const QItemSelection&)), videoView, SLOT(changeSelection(const QItemSelection&, const QItemSelection&)));

	connect(filesView, SIGNAL(currentHasChanged(const QModelIndex&, const QModelIndex&)), videoView, SLOT(changeCurrent(const QModelIndex&, const QModelIndex&)));
	connect(filesView, SIGNAL(currentHasChanged(const QModelIndex&, const QModelIndex&)), songView, SLOT(changeCurrent(const QModelIndex&, const QModelIndex&)));
	connect(videoView, SIGNAL(currentHasChanged(const QModelIndex&, const QModelIndex&)), filesView, SLOT(changeCurrent(const QModelIndex&, const QModelIndex&)));
	connect(videoView, SIGNAL(currentHasChanged(const QModelIndex&, const QModelIndex&)), songView, SLOT(changeCurrent(const QModelIndex&, const QModelIndex&)));
	connect(songView, SIGNAL(currentHasChanged(const QModelIndex&, const QModelIndex&)), filesView, SLOT(changeCurrent(const QModelIndex&, const QModelIndex&)));
	connect(songView, SIGNAL(currentHasChanged(const QModelIndex&, const QModelIndex&)), videoView, SLOT(changeCurrent(const QModelIndex&, const QModelIndex&)));

	connect(filesTab, SIGNAL(projectWasModified()), this, SLOT(projectWasModified()));
	connect(videoTab, SIGNAL(projectWasModified()), this, SLOT(projectWasModified()));
	connect(songTab, SIGNAL(projectWasModified()), this, SLOT(projectWasModified()));
	connect(this, SIGNAL(savedProject()), songTab, SLOT(saveData()));
}

void MateBook::readGuiSettings()
{
	QSettings settings("IMP", "MateBook");
	resize(settings.value("size", QSize(400, 400)).toSize());
	move(settings.value("pos", QPoint(200, 200)).toPoint());

	configDialog->readGuiSettings(settings);

	// most recent directories visited in file dialogs
	lastProjectDirectory = settings.value("lastProjectDirectory", QString()).toString();

	for (int tabIndex = 0; tabIndex != modeTab->count(); ++tabIndex) {
		assert_cast<AbstractTab*>(modeTab->widget(tabIndex))->readGuiSettings(settings);
	}
}

void MateBook::writeGuiSettings()
{
	QSettings settings("IMP", "MateBook");
	settings.setValue("pos", pos());
	settings.setValue("size", size());

	configDialog->writeGuiSettings(settings);

	// most recent directories visited in file dialogs
	settings.setValue("lastProjectDirectory", lastProjectDirectory);

	for (int tabIndex = 0; tabIndex != modeTab->count(); ++tabIndex) {
		assert_cast<AbstractTab*>(modeTab->widget(tabIndex))->writeGuiSettings(settings);
	}
}

bool MateBook::maybeSave()
{
	if (isWindowModified()) {
		QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("MateBook"),
			tr("The project has been modified.\nDo you want to save your changes?"),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (ret == QMessageBox::Save) {
			return saveProject();
		} else if (ret == QMessageBox::Cancel) {
			return false;
		}
	}
	return true;
}

void MateBook::readHeaderSettings()
{
	for (int tabIndex = 0; tabIndex != modeTab->count(); ++tabIndex) {
		assert_cast<AbstractTab*>(modeTab->widget(tabIndex))->readHeaderSettings();
	}
}

void MateBook::writeHeaderSettings() const
{
	for (int tabIndex = 0; tabIndex != modeTab->count(); ++tabIndex) {
		assert_cast<AbstractTab*>(modeTab->widget(tabIndex))->writeHeaderSettings();
	}
}

void MateBook::writeMatlabImportScript() const
{
	QFile::copy(":/mb/matlab/loadProject.m", currentProject->getDirectory().filePath("loadProject.m"));
	QFile::copy(":/mb/matlab/loadVideo.m", currentProject->getDirectory().filePath("loadVideo.m"));
	QString filePath = currentProject->getDirectory().filePath("openWithMATLAB.m");
	std::ofstream out(filePath.toStdString().c_str());
	if (!out) {
		std::cerr << "could not open file for writing: " << filePath.toStdString() << std::endl;
		return;
	}
	// we need this map for translating data type names and number of components to MATLAB-speak, see "doc fread"
	typedef std::map<std::string, std::pair<std::string, unsigned int> > TypeMap;
	TypeMap typeMap;
	typeMap["Vec<float,2>"] = std::make_pair("*float", 2);
	typeMap["float"] = std::make_pair("*float", 1);
	typeMap["uint32_t"] = std::make_pair("*uint32", 1);
	typeMap["MyBool"] = std::make_pair("*uint8", 1);
	{
		out << "frameAttributeInfo = containers.Map;" << "\n";
		FrameAttributes dummy;
		std::vector<std::string> attributeNames = dummy.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			std::string type = dummy.get(*iter).getType();
			TypeMap::const_iterator mapIter = typeMap.find(type);
			if (mapIter == typeMap.end()) {
				std::cerr << "type not found in typeMap: " << type << std::endl;
				continue;
			}
			out << "frameAttributeInfo('" << *iter << "') = struct('typeConversion', '" << mapIter->second.first << "', 'componentCount', " << mapIter->second.second << ");" << "\n";
		}
	}
	{
		out << "flyAttributeInfo = containers.Map;" << "\n";
		FlyAttributes dummy;
		std::vector<std::string> attributeNames = dummy.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			std::string type = dummy.get(*iter).getType();
			TypeMap::const_iterator mapIter = typeMap.find(type);
			if (mapIter == typeMap.end()) {
				std::cerr << "type not found in typeMap: " << type << std::endl;
				continue;
			}
			out << "flyAttributeInfo('" << *iter << "') = struct('typeConversion', '" << mapIter->second.first << "', 'componentCount', " << mapIter->second.second << ");" << "\n";
		}
	}
	{
		out << "pairAttributeInfo = containers.Map;" << "\n";
		PairAttributes dummy;
		std::vector<std::string> attributeNames = dummy.getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			std::string type = dummy.get(*iter).getType();
			TypeMap::const_iterator mapIter = typeMap.find(type);
			if (mapIter == typeMap.end()) {
				std::cerr << "type not found in typeMap: " << type << std::endl;
				continue;
			}
			out << "pairAttributeInfo('" << *iter << "') = struct('typeConversion', '" << mapIter->second.first << "', 'componentCount', " << mapIter->second.second << ");" << "\n";
		}
	}
	out << "trackingResults = loadProject(frameAttributeInfo, flyAttributeInfo, pairAttributeInfo);" << "\n";
}

void MateBook::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat("text/uri-list")) {
		event->acceptProposedAction();
	}
}

void MateBook::dropEvent(QDropEvent* event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	QStringList failedToAdd;
	unsigned int numberOfVideosAdded = 0;
	foreach (QUrl url, urls) {
		QString fileName = url.toLocalFile();
		if (currentProject->addVideo(fileName)) {
			++numberOfVideosAdded;
		} else {
			failedToAdd << fileName;
		}
	}
	if (!failedToAdd.empty()) {
		QMessageBox::warning(this, tr("Adding videos"), tr("Could not read files:\n\n%1\n\nThese are either inaccessible or use an unsupported format.").arg(failedToAdd.join("\n")));
	}
	statusBar()->showMessage(tr("Added %1 videos").arg(numberOfVideosAdded), statusDisplayDuration);
}

bool MateBook::notify(QObject * rec, QEvent * ev)
{
  try{
	  return qApp->notify(rec, ev);
  }catch(std::exception& e){
	  QMessageBox::warning(0, tr("An error occurred"), e.what());
  }
  catch(...){
    QMessageBox::warning(0,tr("An unexpected error occurred"), tr("This is likely a bug."));
  }
  return false;
}
