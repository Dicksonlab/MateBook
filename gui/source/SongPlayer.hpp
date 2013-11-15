#ifndef SongPlayer_hpp
#define SongPlayer_hpp

#include <vector>
#include <string>
#include <map>
#include <QWidget>
#include <QTime>
#include <QVector3D>
#include <boost/shared_ptr.hpp>

#include <phonon/audiooutput.h>
#include <phonon/mediaobject.h>
#include <phonon/volumeslider.h>
#include <phonon/backendcapabilities.h>
#include <Phonon/MediaSource>

#include "Video.hpp"

QT_BEGIN_NAMESPACE
class QSlider;
class QHBoxLayout;
class QPushButton;
class QSpinBox;
class QTimeEdit;
class QLineEdit;
class QAudioFormat;
class QAudioOutput;
class QIODevice;
class QAudioDeviceInfo;
QT_END_NAMESPACE

class QGLGrapher;
class LineGraph;
class MarkerGraph;

//Contains a plot to visualize audio files and buttons
//to navigate in this plot
class SongPlayer : public QWidget
{
	Q_OBJECT

public:
	enum Mode { Play, Pause, Stop };

	SongPlayer(QWidget* parent = 0);
	~SongPlayer();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void setCurrentAudio(boost::shared_ptr<Video> audio);

	void changeColor(std::pair<int, int> indexRange, const std::vector<QVector4D>& newColors);
	void drawCurrentSong();
	void clear();

	std::pair<int, int> getSelection() const;
	void setSelection(std::pair<float, float> range);
	void focusOn(std::pair<size_t, size_t> range);

	MarkerGraph* addMarker(const std::string& imageFileName,
							const float* x,	// the x values where the markers should be placed w/o zooming
							const float* y,	// the y values where the markers should be placed w/o zooming
							size_t count,	// the number of markers
							QVector3D offset = QVector3D(0.0, 0.0, 0.0),
							float scale = 1.0f,	// 1.0 means that 1 pixel is 1 texel
							float rotate = 0.0f	// the rotation of the markers in rads
	);
	void removeMarker(MarkerGraph* markerGraph);

	size_t getCurrentSampleNumber() const;
	size_t getValuesDisplayedCount() const;

signals:
	void selectionChanged(std::pair<int, int> range);

public slots:
	void play();
	void pause();
	void stop();

	void centerFrame(int i); // int because sender uses int
	void currentFrame(size_t i);
	void setNumValues(size_t i);
	void selectedIndexesChanged(std::pair<int, int> indexRange);
	void setPlayerSeconds(int sampleNumber);
	void centerFrameAtTime(const QTime& t);
	void stateChanged(Phonon::State newState, Phonon::State oldState);
	void tick(qint64 time);

private:
	boost::shared_ptr<Video> currentAudio;

	QSlider* timeSlider;
	QPushButton* btnPlay;
	QPushButton* btnNext;
	QPushButton* btnPrevious;
	QPushButton* btnPause;
	QPushButton* btnStop;
	QSpinBox* spinbFrame;
	QTimeEdit* timeBox;
	QLineEdit* frameInSec;
	QLineEdit* selectedAreaMs;
	QGLGrapher* songGrapher;
	LineGraph* audioGraph;

    Phonon::MediaObject *mediaObject;
    Phonon::AudioOutput *audioOutput;
    Phonon::VolumeSlider *volumeSlider;

//	size_t valuesDisplayed;

	void updateSelectedAreaMs();
};

#endif
