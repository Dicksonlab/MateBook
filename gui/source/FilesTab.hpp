#ifndef FilesTab_hpp
#define FilesTab_hpp

#include <QModelIndex>
#include <QString>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "AbstractTab.hpp"
#include "Video.hpp"
#include "FileItem.hpp"

QT_BEGIN_NAMESPACE
class QMenu;
class QTreeView;
class QAbstractItemDelegate;
QT_END_NAMESPACE

class FileItem;
class VideoPlayer;
class FancyTreeView;

class FilesTab : public AbstractTab
{
	Q_OBJECT

public:
	enum Mode { Play, Pause, Stop };
	enum DrawMode { DrawVideo, DrawBackground, DrawDifference };

	FilesTab(MateBook* mateBook, QWidget* parent = 0);
	~FilesTab();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void readGuiSettings(const QSettings& settings);
	void writeGuiSettings(QSettings& settings);
	void readHeaderSettings();
	void writeHeaderSettings();

	void setProject(Project* project);
	void setCurrentItem(Item* item = NULL);
	FancyTreeView* getSharedTreeView() const;

	std::vector<Item*> getSelectedItems() const;
	std::vector<FileItem*> getSelectedFileItems() const;
	std::vector<ArenaItem*> getSelectedArenaItems() const;
	Item* getSelectedItem() const;
	FileItem* getSelectedFileItem() const;
	ArenaItem* getSelectedArenaItem() const;

public slots:
	void enter();
	void leave();

	void cut();
	void copy();
	void paste();
	void del();

	void toBeDeleted(FileItem* fileItem);

private slots:
	void addFile();
	void changeFilePath();

	void runArenaDetection();
	void runFlyTracking();
	void runPostprocessor();
	void runStatisticalVideoAnalysis();
	void runPulseDetection();
	void runStatisticalSongAnalysis();

	void resetArenaDetection();
	void resetFlyTracking();
	void resetStatisticalVideoAnalysis();
	void resetPulseDetection();
	void resetStatisticalSongAnalysis();
	void reloadCleanData();

	void groupAuto();
	void groupSelection();

private:
	bool checkSongOptionIds(const std::vector<FileItem*>& fileItems);	//TODO: shouldn't be here

	MateBook* mateBook;	// to access the settings

	FancyTreeView* filesView;
	std::map<QString, QAbstractItemDelegate*> filesViewDelegates;

	Project* currentProject;

	QString lastMediaDirectory;
};

#endif
