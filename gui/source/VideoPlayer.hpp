#ifndef VideoPlayer_hpp
#define VideoPlayer_hpp

#include <QWidget>
#include <boost/shared_ptr.hpp>
#include "Video.hpp"
#include "VideoRenderer.hpp"

QT_BEGIN_NAMESPACE
class QSlider;
class QPushButton;
class QSpinBox;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
QT_END_NAMESPACE

class VideoPlayer : public QWidget
{
	Q_OBJECT

public:
	enum Mode { Play, Pause, Stop };

	VideoPlayer(QWidget* parent = 0);
	~VideoPlayer();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;
	void setCurrentVideo(boost::shared_ptr<Video> video);
	void setCurrentVideo(boost::shared_ptr<Video> video, unsigned int roiLeft, unsigned int roiTop, unsigned int roiWidth, unsigned int roiHeight);

	size_t getCurrentFrameNumber() const;

	void bindTexture(const QString& fileName);
	void deleteTextures();

	void setTimeSliderImage(const QString& fileName);

signals:
	void drawOverlay();
	void frameDisplayed(size_t frameNumber);
	void finished();
	void clicked(QPoint);
	void doubleClicked(QPoint);
	void drag(QPoint, QPoint);
	void dragFinished(QPoint, QPoint);

public slots:
	void play();
	void pause();
	void stop();
	void seek(int frameNumber);
	void displayCurrentFrame();

private slots:
	void rendererClicked(QPoint);
	void rendererDoubleClicked(QPoint);
	void rendererDrag(QPoint, QPoint);
	void rendererDragFinished(QPoint, QPoint);

	void record(bool enable);
	void displayNextFrame();
	void setSpeed(int percent);
	void bufferAboutToBeSwapped();

private:
	Mode currentMode;
	boost::shared_ptr<Video> currentVideo;

	QVBoxLayout* verticalLayout;
	VideoRenderer* videoRenderer;
	QSlider* timeSlider;
	QHBoxLayout* buttonLayout;
	QPushButton* playButton;
	QPushButton* pauseButton;
	QPushButton* stopButton;
	QPushButton* recordButton;
	QSpinBox* frameSpinBox;
	QSpinBox* speedSpinBox;

	QTimer* frameTimer;

	int seekTo;
	bool seekPending;
	bool playBackward;

	QString lastRecordDirectory;
};

#endif
