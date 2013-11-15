#ifndef VideoTab_hpp
#define VideoTab_hpp

#include <QModelIndex>
#include <QString>
#include <boost/shared_ptr.hpp>
#include "AbstractTab.hpp"
#include "Video.hpp"
#include "VideoPlayer.hpp"
#include "FileItem.hpp"

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QRadioButton;
class QCheckBox;
class QTreeView;
class QSortFilterProxyModel;
class QAbstractItemDelegate;
QT_END_NAMESPACE

class FileItem;
class VideoPlayer;
class FancyTreeView;

class VideoTab : public AbstractTab
{
	Q_OBJECT

public:
	enum Mode { Play, Pause, Stop };
	enum DrawMode { DrawVideo, DrawBackground, DrawDifference };

	VideoTab(QWidget* parent = 0);
	~VideoTab();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void readHeaderSettings();
	void writeHeaderSettings();

	void setProject(Project* project);
	void setCurrentItem(Item* item = NULL);
	FancyTreeView* getSharedTreeView() const;
	ArenaItem* getSelectedArenaItem() const;

signals:
	void moreDetail();

public slots:
	void enter();
	void leave();

	void cut();
	void copy();
	void paste();
	void del();

	void runArenaDetection();
	void runFlyTracking();

	void play();
	void pause();
	void stop();
	void seek(qint64 ms);
	void seek(int frameNumber);

	void toBeDeleted(FileItem* fileItem);

private slots:
	void drawOverlay();
	void clicked(QPoint);
	void doubleClicked(QPoint);
	void drag(QPoint, QPoint);
	void dragFinished(QPoint, QPoint);
	void overlaySettingsChanged();
	void drawModeChanged(int radioButtonId);
	void currentArenaChanged();

private:
	bool selectClickedArena(QPoint);	// returns false if the QPoint is not within arena boundaries
	Item* getItem(const QModelIndex& index) const;

	Mode currentMode;
	FileItem* currentItem;
	Project* currentProject;

	VideoPlayer* videoPlayer;

	QCheckBox* drawArenaCheckBox;
	QCheckBox* drawEthogramCheckBox;
	QButtonGroup* drawModeButtonGroup;
	QRadioButton* drawVideoRadioButton;
	QRadioButton* drawBackgroundRadioButton;
	QRadioButton* drawDifferenceRadioButton;

	FancyTreeView* videoView;
	std::map<QString, QAbstractItemDelegate*> videoViewDelegates;
};

#endif
