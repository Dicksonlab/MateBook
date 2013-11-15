#ifndef ArenaTab_hpp
#define ArenaTab_hpp

#include <QWidget>
#include <QList>
#include <boost/shared_ptr.hpp>
#include "Video.hpp"
#include "VideoPlayer.hpp"
#include "AttributeGrapher.hpp"
#include "AbstractTab.hpp"
#include "../../grapher/source/QGLGrapher.hpp"

QT_BEGIN_NAMESPACE
class QSlider;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
QT_END_NAMESPACE

class ArenaItem;
class QGLGrapher;
class VerticalWidgetList;

class ArenaTab : public AbstractTab
{
	Q_OBJECT

public:
	enum Mode { Play, Pause, Stop };

	ArenaTab(QWidget* parent = 0);
	~ArenaTab();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void setProject(Project* project);
	void setCurrentItem(Item* item = NULL);

public slots:
	void enter();
	void leave();

	void cut();
	void copy();
	void paste();
	void del();

//	void runFlyTracking();

	void play();
	void pause();
	void stop();
	void seek(qint64 ms);
	void seek(int frameNumber);

private slots:
	void rightDisplayComboBoxChanged(int index);

	void plotCenterChanged(size_t newCenter);
	void plotZoomChanged(size_t newNumValues);

	void drawPrimaryOverlay();
	void drawSecondaryOverlay();
	void overlaySettingsChanged();

	void primaryFrameDisplayed(size_t frameNumber);

	void showOcclusion(int occlusionNumber);
	void switchBefore();
	void switchLocally();
	void switchAfter();
	void saveOcclusion();

private:
	void drawArrow(size_t frameNumber);
	void drawBody(size_t frameNumber);
	void drawBocSegments(size_t frameNumber);
	void drawWing(size_t frameNumber);
	void drawContour(size_t contourFileOffset, const QVector4D& color);
	void drawTrail(size_t frameNumber, double timeDelta);
	void drawWingTip(size_t frameNumber);

	Mode currentMode;
	boost::shared_ptr<TrackingResults> currentResults;
	std::vector<QVector4D> flyColors;

	ArenaItem* currentArena;

	QHBoxLayout* playersAndButtonsLayout;

	VideoPlayer* primaryPlayer;
	VideoPlayer* secondaryPlayer;
	QGLGrapher* grapher;

	QComboBox* rightDisplayComboBox;
	QCheckBox* leftDrawArrowCheckBox;
	QCheckBox* leftDrawBodyCheckBox;
	QCheckBox* leftDrawBocSegmentsCheckBox;
	QCheckBox* leftDrawWingCheckBox;
	QDoubleSpinBox* leftTrailSpinBox;
	QCheckBox* rightDrawArrowCheckBox;
	QCheckBox* rightDrawBodyCheckBox;
	QCheckBox* rightDrawWingCheckBox;
	QDoubleSpinBox* rightTrailSpinBox;

	QPushButton* previousOcclusionButton;
	QPushButton* nextOcclusionButton;
	QSpinBox* occlusionSpinBox;
	QPushButton* saveOcclusionButton;

	QSpinBox* plotSpinBox;
	VerticalWidgetList* plots;
	QWidget* createAttributePlot();

	bool hasBeenEntered;
	bool occlusionsModified;
};

#endif
