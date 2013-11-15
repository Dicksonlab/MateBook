#include <QStyle>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVector4D>
#include <QString>
#include <QTimeEdit>
#include <QLineEdit>
#include <QTime>
#include <QMessageBox>
#include <QProgressDialog>

#include <iostream>
#include <boost/lexical_cast.hpp>
#include "../../grapher/source/QGLGrapher.hpp"
#include "SongPlayer.hpp"
#include "AttributeGrapher.hpp"
#include "../../common/source/macro.h"

SongPlayer::SongPlayer(QWidget* parent) : QWidget(parent),
audioGraph(NULL)
{
	// ==================== Init Audio
	audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
	mediaObject = new Phonon::MediaObject(this);
	volumeSlider = new Phonon::VolumeSlider(this);

	mediaObject->setTickInterval(50);
	volumeSlider->setAudioOutput(audioOutput);
	volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)), this, SLOT(stateChanged(Phonon::State,Phonon::State)));
	connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));

	Phonon::createPath(mediaObject, audioOutput);

	// ==================== Audio Navigation
	const int btnWidth=80;
	timeSlider = new QSlider(Qt::Horizontal);

	btnPlay = new QPushButton("Play");
	btnPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	btnPlay->setFixedWidth(btnWidth);

	btnPause = new QPushButton("Pause");
	btnPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
	btnPause->setFixedWidth(btnWidth);

	btnStop = new QPushButton("Stop");
	btnStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	btnStop->setFixedWidth(btnWidth);

	spinbFrame = new QSpinBox;
	spinbFrame->setMinimum(0);
	spinbFrame->setMaximum(0);
	spinbFrame->setPrefix("at sample: ");
	spinbFrame->setFixedWidth(140);

	timeBox = new QTimeEdit();
	timeBox->setFixedWidth(140);
	timeBox->setDisplayFormat("hh:mm:ss:zzz");

	timeBox = new QTimeEdit();
	timeBox->setFixedWidth(140);
	timeBox->setDisplayFormat("hh:mm:ss:zzz");

	frameInSec = new QLineEdit();
	frameInSec->setFixedWidth(140);
	frameInSec->setReadOnly(true);
	frameInSec->setText("cursor at: 0 sec");

	selectedAreaMs = new QLineEdit();
	selectedAreaMs->setFixedWidth(140);
	selectedAreaMs->setReadOnly(true);
	selectedAreaMs->setText("selection: 0 ms");

	// ==================== Layout
	QHBoxLayout* buttonLayoutPlayer = new QHBoxLayout;
	buttonLayoutPlayer->addWidget(btnPlay);
	buttonLayoutPlayer->addWidget(btnPause);
	btnPause->hide();
	buttonLayoutPlayer->addWidget(btnStop);
	buttonLayoutPlayer->addWidget(volumeSlider);
	buttonLayoutPlayer->setAlignment(Qt::AlignLeft);

	QHBoxLayout* buttonLayoutSong = new QHBoxLayout;
	buttonLayoutSong->addWidget(spinbFrame);
	buttonLayoutSong->addWidget(timeBox);
	buttonLayoutSong->addWidget(frameInSec);
	buttonLayoutSong->addWidget(selectedAreaMs);
	buttonLayoutSong->setAlignment(Qt::AlignRight);

	QHBoxLayout* buttonLayout = new QHBoxLayout;
	buttonLayout->addLayout(buttonLayoutPlayer);
	buttonLayout->addLayout(buttonLayoutSong);

	QVBoxLayout* verticalLayout = new QVBoxLayout;
	songGrapher = new QGLGrapher("sample", this);
	songGrapher->setAntiAliasing(true);
	songGrapher->setFontSize(20);
	songGrapher->setAutoScale(true);
	songGrapher->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0), 1, QVector3D(0, 0, -0.1));	//TODO: fix grapher near- and far-plane

	verticalLayout->addWidget(songGrapher);
	verticalLayout->addWidget(timeSlider);
	verticalLayout->addLayout(buttonLayout);
	setLayout(verticalLayout);

	// ==================== Button Actions
	connect(timeSlider, SIGNAL(valueChanged(int)), this, SLOT(centerFrame(int)));
	connect(btnPlay, SIGNAL(clicked(bool)), this, SLOT(play()));
	connect(btnPause, SIGNAL(clicked(bool)), this, SLOT(pause()));
	connect(btnStop, SIGNAL(clicked(bool)), this, SLOT(stop()));
	connect(spinbFrame, SIGNAL(valueChanged(int)), this, SLOT(centerFrame(int)));
	connect(songGrapher, SIGNAL(centerChanged(size_t)), this, SLOT(currentFrame(size_t)));
	connect(songGrapher, SIGNAL(currentSelectedIndexes(std::pair<int, int>)), this, SLOT(selectedIndexesChanged(std::pair<int, int>)));
	connect(spinbFrame, SIGNAL(valueChanged(int)), this, SLOT(setPlayerSeconds(int)));
	connect(timeBox, SIGNAL(timeChanged(QTime)), this, SLOT(centerFrameAtTime(QTime)));
}

SongPlayer::~SongPlayer()
{
}

QSize SongPlayer::minimumSizeHint() const
{
	return QSize(80, 120);
}

QSize SongPlayer::sizeHint() const
{
	return QSize(320, 480);
}

// set a data vector (samples displayed by plot) and its sample  rate
void SongPlayer::setCurrentAudio(boost::shared_ptr<Video> audio)
{
	currentAudio = audio;

	if (mediaObject != 0) {
		mediaObject->setCurrentSource(QString::fromStdString(currentAudio->getFileName()));
	}

	spinbFrame->setMaximum(currentAudio->getSong().size());
	timeSlider->setRange(0, currentAudio->getSong().size());
}

// changes the current graphs color within the given interval
void SongPlayer::changeColor(std::pair<int, int> indexRange, const std::vector<QVector4D>& newColors)
{
	audioGraph->changeColor(newColors.begin(), newColors.end(), indexRange.first);
	audioGraph->generateIndicesAndRegenerateVertexBuffer();
	songGrapher->redraw();	// otherwise we would have to move the mouse
}

// centers and selects the given range
void SongPlayer::focusOn(std::pair<size_t, size_t> range)
{
	songGrapher->center(range.first);
	spinbFrame->setValue(range.first);
	setSelection(range);
}

MarkerGraph* SongPlayer::addMarker(const std::string& imageFileName, const float* x, const float* y, size_t count, QVector3D offset, float scale,float rotate)
{
	return songGrapher->addMarker(imageFileName, x, y, count, offset, scale, rotate);
}

void SongPlayer::removeMarker(MarkerGraph* markerGraph)
{
	songGrapher->removeData(markerGraph);
}

// sets the current song data (plot) and creates a black color vector in the plot
void SongPlayer::drawCurrentSong()
{
	if (currentAudio->getSong().size() > 0) {
		if (audioGraph) {
			songGrapher->removeData(audioGraph);
			audioGraph = NULL;
		}
		audioGraph = songGrapher->addLineGraph(currentAudio->getSong().begin(), currentAudio->getSong().end(), QVector4D(0, 0, 0, 1), 1, QVector3D(0, 0, -0.3), "song");
	}
}

// deletes all data in the plot
void SongPlayer::clear()
{
	songGrapher->clearSelection();
	songGrapher->clear();
	audioGraph = NULL;

	currentAudio.reset();
}

std::pair<int, int> SongPlayer::getSelection() const
{
	std::pair<int, int> selectedIndexes = songGrapher->getSelection();
	if(selectedIndexes.first > selectedIndexes.second){
		std::swap(selectedIndexes.first, selectedIndexes.second);
	}
	return selectedIndexes;
}

void SongPlayer::setSelection(std::pair<float, float> range)
{
	songGrapher->setSelectedArea(range);
	updateSelectedAreaMs();
}

size_t SongPlayer::getCurrentSampleNumber() const
{
	return songGrapher->getCenter();
}

size_t SongPlayer::getValuesDisplayedCount() const
{
	return songGrapher->getNumValues();
}

void SongPlayer::play()
{
	if(mediaObject != 0){
		mediaObject->play();
	}
}

void SongPlayer::pause()
{
	if(mediaObject != 0){
		mediaObject->pause();
	}
}

void SongPlayer::stop()
{
	if(mediaObject != 0){
		mediaObject->stop();
	}
	timeSlider->setValue(0);
}

// set displayed center to certain frame
void SongPlayer::centerFrame(int i)
{
	songGrapher->center(i);
	currentFrame(i);
	if(mediaObject != 0 && mediaObject->isSeekable()){
		qint64 millisec = 1000.0 / currentAudio->getSampleRate() * i;
		mediaObject->seek(millisec);
	}
}

// sets the spinBox and the slider to the current frame
void SongPlayer::currentFrame(size_t i)
{
	spinbFrame->setValue(i);
	timeSlider->setValue(i);
}

// change the number of displayed values on screen
void SongPlayer::setNumValues(size_t i)
{
	songGrapher->setNumValues(i);
}

// receives a std::pair with start and end index of an selected area
void SongPlayer::selectedIndexesChanged(std::pair<int, int> indexRange)
{
	updateSelectedAreaMs();
	emit selectionChanged(indexRange);
}

// calculate the time of the current frame and set timeBox value accordingly
void SongPlayer::setPlayerSeconds(int sampleNumber)
{
	QString seconds;
	seconds.setNum(1.0 / currentAudio->getSampleRate() * sampleNumber);
	frameInSec->setText("cursor at: " + seconds + " sec");

	QTime zero(0,0,0,0);
	QTime msec = zero.addMSecs(static_cast<int>(1000.0 / currentAudio->getSampleRate() * sampleNumber));
	timeBox->setTime(msec);
}

// center frame at the current time displayed in the timeBox
void SongPlayer::centerFrameAtTime(const QTime& t)
{
	int hours = t.hour();
	int min = (hours + t.minute())*60;
	int sec = (min + t.second())*1000;
	int frame = currentAudio->getSampleRate() / 1000.0 * (sec + t. msec());

	if(frame > currentAudio->getSong().size()){
		frame = currentAudio->getSong().size();
	}
	centerFrame(frame);
}

void SongPlayer::stateChanged(Phonon::State newState, Phonon::State oldState)
{
    switch (newState) {
        case Phonon::ErrorState:
            if (mediaObject->errorType() == Phonon::FatalError) {
                QMessageBox::warning(this, tr("Fatal Error"),
                mediaObject->errorString());
            } else {
                QMessageBox::warning(this, tr("Error"),
                mediaObject->errorString());
            }
            break;
        case Phonon::PlayingState:
                btnPlay->hide();
				btnPause->show();
                break;
        case Phonon::StoppedState:
				btnPause->hide();
                btnPlay->show();
                break;
        case Phonon::PausedState:
				btnPause->hide();
                btnPlay->show();
                break;
        case Phonon::BufferingState:
                break;
        default:
            ;
    }
}

void SongPlayer::tick(qint64 time)
{
	if (!currentAudio) {
		return;
	}
	int sample = time*currentAudio->getSampleRate()/1000;
	setPlayerSeconds(sample);
}

void SongPlayer::updateSelectedAreaMs()
{
	selectedAreaMs->setText("selection: " + QString::number(1000.0 / currentAudio->getSampleRate() * (getSelection().second - getSelection().first)) + " ms");
}
