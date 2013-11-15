#include <QtGui>

#include <cmath>
#include <sstream>
#include <iostream>
#include "VideoPlayer.hpp"

VideoPlayer::VideoPlayer(QWidget* parent) : QWidget(parent),
	currentMode(Stop),
	currentVideo(),
	seekTo(),
	seekPending(false),
	playBackward(false)
{
	videoRenderer = new VideoRenderer(this);
	timeSlider = new QSlider(Qt::Horizontal);

	playButton = new QPushButton("Play");
	playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	pauseButton = new QPushButton("Pause");
	pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
	stopButton = new QPushButton("Stop");
	stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	recordButton = new QPushButton("Record");
	recordButton->setCheckable(true);
	frameSpinBox = new QSpinBox;
	frameSpinBox->setPrefix("Frame: ");
	speedSpinBox = new QSpinBox;
	speedSpinBox->setPrefix("Speed: ");
	speedSpinBox->setSuffix("%");
	speedSpinBox->setRange(-200, 200);
	speedSpinBox->setSingleStep(25);
	speedSpinBox->setValue(100);

	buttonLayout = new QHBoxLayout;
	buttonLayout->addWidget(playButton);
	buttonLayout->addWidget(pauseButton);
	pauseButton->hide();
	buttonLayout->addWidget(stopButton);
	buttonLayout->addWidget(recordButton);
	buttonLayout->addWidget(frameSpinBox);
	buttonLayout->addWidget(speedSpinBox);

	verticalLayout = new QVBoxLayout;
	verticalLayout->addWidget(videoRenderer);
	verticalLayout->addWidget(timeSlider);
	verticalLayout->addLayout(buttonLayout);
	setLayout(verticalLayout);

	frameTimer = new QTimer(this);
	connect(frameTimer, SIGNAL(timeout()), this, SLOT(displayNextFrame()));

	connect(playButton, SIGNAL(clicked(bool)), this, SLOT(play()));
	connect(pauseButton, SIGNAL(clicked(bool)), this, SLOT(pause()));
	connect(stopButton, SIGNAL(clicked(bool)), this, SLOT(stop()));
	connect(recordButton, SIGNAL(toggled(bool)), this, SLOT(record(bool)));

	connect(timeSlider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
	connect(frameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));

	connect(speedSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setSpeed(int)));

	connect(videoRenderer, SIGNAL(bufferAboutToBeSwapped()), this, SLOT(bufferAboutToBeSwapped()));
	
	connect(videoRenderer, SIGNAL(clicked(QPoint)), this, SLOT(rendererClicked(QPoint)));
	connect(videoRenderer, SIGNAL(doubleClicked(QPoint)), this, SLOT(rendererDoubleClicked(QPoint)));
	connect(videoRenderer, SIGNAL(drag(QPoint, QPoint)), this, SLOT(rendererDrag(QPoint, QPoint)));
	connect(videoRenderer, SIGNAL(dragFinished(QPoint, QPoint)), this, SLOT(rendererDragFinished(QPoint, QPoint)));
}

VideoPlayer::~VideoPlayer()
{
}

QSize VideoPlayer::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize VideoPlayer::sizeHint() const
{
	return QSize(640, 480);
}

void VideoPlayer::setCurrentVideo(boost::shared_ptr<Video> video)
{
	if (!video) {
		return;
	}else if(video->isVideo()){
		pause();
		frameTimer->stop();
		videoRenderer->setCurrentVideo(video);
		timeSlider->setRange(0, video->getNumFrames() - 1);
		frameSpinBox->setRange(0, video->getNumFrames() - 1);
		currentVideo = video;
		setSpeed(speedSpinBox->value());
		frameTimer->start();
	}else{
		QMessageBox::critical(this, tr("Wrong format: "), "File is not a video file");
	}
}

void VideoPlayer::setCurrentVideo(boost::shared_ptr<Video> video, unsigned int roiLeft, unsigned int roiTop, unsigned int roiWidth, unsigned int roiHeight)
{
	if (!video) {
		return;
	}else if(video->isVideo()){
		pause();
		frameTimer->stop();
		videoRenderer->setCurrentVideo(video, roiLeft, roiTop, roiWidth, roiHeight);
		timeSlider->setRange(0, video->getNumFrames() - 1);
		frameSpinBox->setRange(0, video->getNumFrames() - 1);
		currentVideo = video;
		setSpeed(speedSpinBox->value());
		frameTimer->start();
	}else{
		QMessageBox::critical(this, tr("Wrong format: "), "File is not a video file");
	}
}

size_t VideoPlayer::getCurrentFrameNumber() const
{
	if (!currentVideo) {
		return 0;
	}

	return currentVideo->getCurrentFrameNumber();
}

void VideoPlayer::bindTexture(const QString& fileName)
{
	videoRenderer->bindExtraTexture(fileName);
}

void VideoPlayer::deleteTextures()
{
	videoRenderer->deleteExtraTextures();
}

QString cssEscape(const QString& unescaped)
{
	std::string unescapedStdString = unescaped.toStdString();
	std::ostringstream os;
	os << std::hex;
	for (size_t i = 0; i != unescapedStdString.size(); ++i) {
		os << "\\" << static_cast<unsigned int>(unescapedStdString[i]);
	}
	return QString::fromStdString(os.str());
}

void VideoPlayer::setTimeSliderImage(const QString& fileName)
{
	timeSlider->setStyleSheet(" \
		QSlider::groove:horizontal { \
			border: 1px solid #999999; \
			height: 12px; \
			background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4); \
			border-image: url('" + cssEscape(fileName) + "') 0 8 0 8; \
			border-top: 0px transparent; \
			border-bottom: 0px transparent; \
			border-right: 8px transparent; \
			border-left: 8px transparent; \
			margin: 2px 0; \
		} \
		QSlider::handle:horizontal { \
			background-image: url(:/mb/icons/ethoSliderHandle.png); \
			border: 1px solid #5c5c5c; \
			width: 18px; \
			margin: -2px -8px; \
			border-radius: 3px; \
		} \
	");
}

void VideoPlayer::play()
{
	if (!currentVideo) {
		return;
	}

	playButton->hide();
	pauseButton->show();
	currentMode = Play;
}

void VideoPlayer::pause()
{
	if (!currentVideo) {
		return;
	}

	pauseButton->hide();
	playButton->show();
	currentMode = Pause;
}

void VideoPlayer::stop()
{
	if (!currentVideo) {
		return;
	}

	pauseButton->hide();
	playButton->show();
	currentMode = Stop;
	seek(0);
}

void VideoPlayer::seek(int frameNumber)
{
	if (!currentVideo) {
		return;
	}

	seekTo = frameNumber;
	seekPending = true;
}

void VideoPlayer::displayCurrentFrame()
{
	if (!currentVideo) {
		return;
	}

	videoRenderer->updateGL();
	timeSlider->blockSignals(true);
	timeSlider->setValue(currentVideo->getCurrentFrameNumber());
	timeSlider->blockSignals(false);
	frameSpinBox->blockSignals(true);
	frameSpinBox->setValue(currentVideo->getCurrentFrameNumber());
	frameSpinBox->blockSignals(false);
	emit frameDisplayed(currentVideo->getCurrentFrameNumber());
}

void VideoPlayer::rendererClicked(QPoint point)
{
	emit clicked(point);
}

void VideoPlayer::rendererDoubleClicked(QPoint point)
{
	emit doubleClicked(point);
}

void VideoPlayer::rendererDrag(QPoint start, QPoint current)
{
	emit drag(start, current);
}

void VideoPlayer::rendererDragFinished(QPoint start, QPoint end)
{
	emit dragFinished(start, end);
}

void VideoPlayer::record(bool enable)
{
	if (!currentVideo) {
		return;
	}

	if (enable) {
		QString fileName = QFileDialog::getSaveFileName(this, tr("Record Video"), lastRecordDirectory, tr("Video Files (*.avi);;All Files (*.*)"));
		if (fileName.isEmpty()) {
			return;
		}
		if (!fileName.endsWith(".avi", Qt::CaseInsensitive)) {
			fileName += ".avi";
		}
		lastRecordDirectory = fileName;
		videoRenderer->startRecording(fileName);
	} else {
		videoRenderer->stopRecording();
	}
}

void VideoPlayer::displayNextFrame()
{
	if (seekPending) {
//		std::cout << "seeking to frame " << seekTo << std::endl;
		currentVideo->seek(seekTo);
		seekPending = false;
		displayCurrentFrame();
	} else if (currentMode == Play) {
		unsigned int currentFrameNumber = currentVideo->getCurrentFrameNumber();
		if (playBackward) {
			if (currentFrameNumber != 0) {
				currentVideo->seek(currentFrameNumber - 1);
			}
		} else {
			currentVideo->seek(currentFrameNumber + 1);
		}
		displayCurrentFrame();
	}
}

void VideoPlayer::setSpeed(int percent)
{
	if (!currentVideo) {
		return;
	}
	
	int msFrameTime = static_cast<int>(currentVideo->getFrameTime() * 1000.0 * 100.0 / percent);
	if (msFrameTime > 0) {
		playBackward = false;
	} else if (msFrameTime == 0) {
		//TODO: handle this somehow?
	} else if (msFrameTime < 0) {
		playBackward = true;
		msFrameTime = -msFrameTime;
	}
	frameTimer->setInterval(msFrameTime);
}

void VideoPlayer::bufferAboutToBeSwapped()
{
	emit drawOverlay();
}
