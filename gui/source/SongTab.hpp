#ifndef SongTab_hpp
#define SongTab_hpp

#include <QTreeView>
#include <QTableView>
#include <QKeyEvent>
#include <QVector4D>

#include <boost/shared_ptr.hpp>
#include "SongResults.hpp"
#include "AbstractTab.hpp"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPushButton;
class QKeyEvent;
class QStandardItemModel;
class QModelIndex;
class QSortFilterProxyModel;
QT_END_NAMESPACE

class Video;
class SongPlayer;
class Project;
class SongStatisticsViewer;
class SongResults;
class FileItem;
class FancyTreeView;
class MarkerGraph;
class MateBook;

//Song-Tab View: To analyse an audio file
class SongTab : public AbstractTab
{
	Q_OBJECT

public:
	enum Mode { SongDetails, PulseDetails, SongStatistics };

	SongTab(MateBook* mateBook, QWidget* parent = 0);

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void readHeaderSettings();
	void writeHeaderSettings();

	void setProject(Project* project);
	void setCurrentItem(Item* item = NULL);
	FancyTreeView* getSharedTreeView() const;

signals:
	void plotDataChanged(); // signal to update TreeView data

public slots:
	void enter();
	void leave();

	void cut();
	void copy();
	void paste();
	void del();

	void runPulseDetection();

	void toBeDeleted(FileItem* fileItem);

	void next();
	void previous();

	void createPulse();
	void deleteSelectedPulses();

	void createSine();
	void deleteSelectedSines();

	void saveData();
	void reloadData();
	void redrawSong();
	void startStatisticalAnalysis();
	void selectPulse(const QModelIndex & index);
	void selectSine(const QModelIndex & index);
	void loadNextSong();
	void loadPrevSong();
	void zoomOut();
	void deleteAllData();

private slots:
	void graphSelectionChanged(std::pair<int, int> range);
	void redrawGraphColors();

private:
	void selectTableRow(unsigned int pulseStart);

	void drawColors(std::pair<int, int> indexRange);
	void redrawMarkers();

	MateBook* mateBook;	// to access the song analysis settings

	bool wasEntered;
	Mode currentMode;
	SongStatisticsViewer* statisticsPlot;

	FancyTreeView* songView;
	Project* currentProject;
	FileItem* currentItem;
	boost::shared_ptr<Video> songItem;
	std::map<QString, QAbstractItemDelegate*> songViewDelegates;

	SongResults songResults;

	QTableView* pulseDetailsTable;
	QTableView* trainDetailsTable;
	QTableView* ipiDetailsTable;
	QTableView* sineDetailsTable;
	QTableView* statisticsTable;

	SongPlayer* songPlayer;
	MarkerGraph* pulseCenterMarkers;

	QStandardItemModel* pulseDetailsModel;
	QStandardItemModel* trainDetailsModel;
	QStandardItemModel* ipiDetailsModel;
	QStandardItemModel* sineDetailsModel;
	QStandardItemModel* statisticsModel;

	QCheckBox* highlightPulsesCheckBox;
	QCheckBox* highlightSinesCheckBox;
	QCheckBox* highlightTrainsCheckBox;

	QPushButton* btnZoomOut;
	QPushButton* btnSaveData;
	QPushButton* btnReloadData;
	QPushButton* btnDeleteSelectedPulses;
	QPushButton* btnCreatePulse;
	QPushButton* btnSongStatistics;
	QPushButton* btnDeleteAllData;

	Item* getItem(const QModelIndex& index) const;

	void dataChanged();
	void fillDetailsModels();
	void fillStatisticsModel();
	
	QString isValidValue(const float number);

	void keyPressEvent(QKeyEvent *e);

	std::vector<QVector4D> drawingPad;	// used by drawColors to set the colors of a range so they can be passed to SongPlayer
	std::vector<float> markerXTemp;	// used by redrawMarkers to set coordinates so they can be passed to SongPlayer
	std::vector<float> markerYTemp;	// used by redrawMarkers to set coordinates so they can be passed to SongPlayer
};

#endif
