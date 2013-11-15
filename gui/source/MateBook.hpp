#ifndef MateBook_hpp
#define MateBook_hpp

#include <QMainWindow>
#include <QModelIndex>
#include <boost/shared_ptr.hpp>
#include "../../mediawrapper/source/mediawrapper.hpp"
#include "../../common/source/Settings.hpp"

QT_BEGIN_NAMESPACE
class QAction;
class QWidgetAction;
class QCheckBox;
class QMenu;
class QPlainTextEdit;
class QTreeView;
class QTimer;
class QSplitter;
class QItemSelection;
class QFormLayout;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QButtonGroup;
class QLineEdit;
class QGroupBox;
QT_END_NAMESPACE

class FilesTab;
class VideoTab;
class SongTab;
class JobQueue;
class Project;
class FileItem;
class ArenaTab;
class StatisticsTab;
class ColorButton;
class ConfigDialog;

class MateBook : public QMainWindow {
	Q_OBJECT

public:
	enum Mode { FilesMode = 0, VideoMode, ArenaMode, SongMode, StatisticsMode };
	MateBook(QWidget* parent = 0, Qt::WFlags flags = 0);
	~MateBook();

	boost::shared_ptr<Project> getCurrentProject() const;
	void saveAsWasSuccessful();

	void readTrackerSettings(const QString& fileName);
	void writeTrackerSettings(const QString& fileName) const;

	ConfigDialog* getConfigDialog();

signals:
	void savedProject();
	void closeMateBook();

protected:
	void closeEvent(QCloseEvent* event);
	void dragEnterEvent(QDragEnterEvent* event);
	void dropEvent(QDropEvent* event);
	bool notify(QObject* rec, QEvent* ev);

private slots:
	void newProject();
	void openProject();
	void openProject(const QString& fileName);
	bool saveProject();
	bool saveProjectAs();
	void showConfigDialog();
	void about();
	void projectWasModified();
	void changeMode(int tabIndex);
	void cut();
	void copy();
	void paste();
	void del();
	void updateSelectedItemState();
	void showHelpBrowser();
	void bugReport();
	void setStatusMessage(const QString& text);
	void moreDetail();	// only works for AbstractTabs that have been added to the modeTab

private:
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();
	void createStyles();
	void createTabs();
	void readGuiSettings();
	void writeGuiSettings();
	bool maybeSave();

	void readHeaderSettings();
	void writeHeaderSettings() const;

	void writeMatlabImportScript() const;

	void addRowToFormLayout(QFormLayout* layout, const QString& key, const QString& labelText, double defaultValue, double minValue, double maxValue, const QString& unit = QString(), const QString& labelToolTip = QString(), const QString& fieldToolTip = QString());
	void setCheckedId(QButtonGroup* buttonGroup, int id);	// helper function to use QButtonGroup with Settings

	boost::shared_ptr<Project> currentProject;

	QTabWidget* modeTab;
	
	QMenu* fileMenu;
	QMenu* editMenu;
	QMenu* helpMenu;
	
	QToolBar* fileToolBar;
	QToolBar* editToolBar;
	
	QAction* newProjectAction;
	QAction* openProjectAction;
	QAction* saveProjectAction;
	QAction* saveProjectAsAction;
	QAction* delAction;
	QAction* exitAction;
	QAction* cutAction;
	QAction* copyAction;
	QAction* pasteAction;
	QAction* showConfigDialogAction;
	QAction* aboutAction;
	QAction* showHelpBrowserAction;
	QAction* bugReportAction;

	ConfigDialog* configDialog;

	// most recent directories visited in file dialogs
	QString lastProjectDirectory;

	Mode currentMode;

	Settings trackerSettings;

	static const int statusDisplayDuration = 5000;
};

#endif
