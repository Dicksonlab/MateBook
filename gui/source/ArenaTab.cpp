#if defined(WIN32) || defined(LINUX)
	#include <GL/gl.h>
#else
	#include <QtOpenGL>
#endif

#include <QtGui>

#include <cmath>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <boost/bind.hpp>
#include "ArenaTab.hpp"
#include "../../grapher/source/QGLGrapher.hpp"
#include "ArenaItem.hpp"
#include "RuntimeError.hpp"
#include "makeColorMap.hpp"
#include "GraphicsPrimitives.hpp"
#include "VerticalWidgetList.hpp"

template<class T>
int sign(const T& value)
{
	return (value > T()) - (value < T());
}

ArenaTab::ArenaTab(QWidget* parent) : AbstractTab(parent),
	currentMode(Stop),
	currentArena(NULL),
	hasBeenEntered(false),
	occlusionsModified(false)
{
	QSplitter* splitter = new QSplitter;
	splitter->setOrientation(Qt::Vertical);

	QWidget* playersAndButtons = new QWidget;
	playersAndButtonsLayout = new QHBoxLayout;
	primaryPlayer = new VideoPlayer(this);
	secondaryPlayer = new VideoPlayer(this);
	grapher = new QGLGrapher("log(difference)", this);
	grapher->setAntiAliasing(false);
	grapher->setFontSize(20);
	playersAndButtonsLayout->addWidget(primaryPlayer, 1);
	// add the exchangeable widgets here and hide them by default
	playersAndButtonsLayout->addWidget(secondaryPlayer, 1);
//	secondaryPlayer->hide();
	playersAndButtonsLayout->addWidget(grapher, 1);
	grapher->hide();

	QVBoxLayout* buttonsLayout = new QVBoxLayout;
	buttonsLayout->setAlignment(Qt::AlignTop);

	rightDisplayComboBox = new QComboBox(this);
	// add an entry for each of the exchangeable widgets above
	rightDisplayComboBox->addItem("Show Main Video Player Only");
	rightDisplayComboBox->addItem("Show First Frame After Occlusion");
	rightDisplayComboBox->addItem("Show Difference-Image Histogram");
	rightDisplayComboBox->setCurrentIndex(1);
	buttonsLayout->addWidget(rightDisplayComboBox);

	QGroupBox* leftOverlaysGroupBox = new QGroupBox(tr("Left Player Overlays"));
	leftOverlaysGroupBox->setFlat(true);
	QVBoxLayout* leftOverlaysGroupBoxLayout = new QVBoxLayout;
	leftDrawArrowCheckBox = new QCheckBox("Arrows");
	leftDrawArrowCheckBox->setChecked(true);
	leftDrawBodyCheckBox = new QCheckBox("Body Contours");
	leftDrawBocSegmentsCheckBox = new QCheckBox("Boc Segments");
	leftDrawWingCheckBox = new QCheckBox("Wing Contours");
	leftTrailSpinBox = new QDoubleSpinBox;
	leftTrailSpinBox->setDecimals(1);
	leftTrailSpinBox->setSingleStep(0.1);
	leftTrailSpinBox->setRange(-1000, 1000);
	leftTrailSpinBox->setPrefix("Trails: ");
	leftTrailSpinBox->setSuffix(" s");
	leftOverlaysGroupBoxLayout->addWidget(leftDrawArrowCheckBox);
	leftOverlaysGroupBoxLayout->addWidget(leftDrawBodyCheckBox);
	leftOverlaysGroupBoxLayout->addWidget(leftDrawBocSegmentsCheckBox);
	leftOverlaysGroupBoxLayout->addWidget(leftDrawWingCheckBox);
	leftOverlaysGroupBoxLayout->addWidget(leftTrailSpinBox);
	leftOverlaysGroupBox->setLayout(leftOverlaysGroupBoxLayout);
	buttonsLayout->addWidget(leftOverlaysGroupBox);

	QGroupBox* rightOverlaysGroupBox = new QGroupBox(tr("Right Player Overlays"));
	rightOverlaysGroupBox->setFlat(true);
	QVBoxLayout* rightOverlaysGroupBoxLayout = new QVBoxLayout;
	rightDrawArrowCheckBox = new QCheckBox("Arrows");
	rightDrawArrowCheckBox->setChecked(true);
	rightDrawBodyCheckBox = new QCheckBox("Body Contours");
	rightDrawWingCheckBox = new QCheckBox("Wing Contours");
	rightTrailSpinBox = new QDoubleSpinBox;
	rightTrailSpinBox->setDecimals(1);
	rightTrailSpinBox->setSingleStep(0.1);
	rightTrailSpinBox->setRange(-1000, 1000);
	rightTrailSpinBox->setPrefix("Trails: ");
	rightTrailSpinBox->setSuffix(" s");
	rightOverlaysGroupBoxLayout->addWidget(rightDrawArrowCheckBox);
	rightOverlaysGroupBoxLayout->addWidget(rightDrawBodyCheckBox);
	rightOverlaysGroupBoxLayout->addWidget(rightDrawWingCheckBox);
	rightOverlaysGroupBoxLayout->addWidget(rightTrailSpinBox);
	rightOverlaysGroupBox->setLayout(rightOverlaysGroupBoxLayout);
	buttonsLayout->addWidget(rightOverlaysGroupBox);

	QGroupBox* occlusionGroupBox = new QGroupBox(tr("Occlusions"));
	occlusionGroupBox->setFlat(true);
	QVBoxLayout* occlusionGroupBoxLayout = new QVBoxLayout;

	QHBoxLayout* previousNextLayout = new QHBoxLayout;
	previousOcclusionButton = new QPushButton;
	previousOcclusionButton->setToolTip("previous");
	previousOcclusionButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
	nextOcclusionButton = new QPushButton;
	nextOcclusionButton->setToolTip("next");
	nextOcclusionButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
	previousNextLayout->addWidget(previousOcclusionButton);
	previousNextLayout->addWidget(nextOcclusionButton);

	QHBoxLayout* spinSaveLayout = new QHBoxLayout;
	occlusionSpinBox = new QSpinBox();
	saveOcclusionButton = new QPushButton("save");
	spinSaveLayout->addWidget(occlusionSpinBox);
	spinSaveLayout->addWidget(saveOcclusionButton);

	QHBoxLayout* switchLayout = new QHBoxLayout;
	QPushButton* switchBeforeButton = new QPushButton("<-|");
	switchBeforeButton->setToolTip("switch before");
	QPushButton* switchLocallyButton = new QPushButton("|<->|");
	switchLocallyButton->setToolTip("switch locally");
	QPushButton* switchAfterButton = new QPushButton("|->");
	switchAfterButton->setToolTip("switch after");
	switchLayout->addWidget(switchBeforeButton);
	switchLayout->addWidget(switchLocallyButton);
	switchLayout->addWidget(switchAfterButton);

	occlusionGroupBoxLayout->addLayout(spinSaveLayout);
	occlusionGroupBoxLayout->addLayout(previousNextLayout);
	occlusionGroupBoxLayout->addLayout(switchLayout);
	occlusionGroupBox->setLayout(occlusionGroupBoxLayout);
	buttonsLayout->addWidget(occlusionGroupBox);

	QGroupBox* plotGroupBox = new QGroupBox(tr("Plots"));
	plotGroupBox->setFlat(true);
	QVBoxLayout* plotGroupBoxLayout = new QVBoxLayout;
	plotSpinBox = new QSpinBox();
	plotSpinBox->setMaximum(9);
	plotGroupBoxLayout->addWidget(plotSpinBox);
	plotGroupBox->setLayout(plotGroupBoxLayout);
	buttonsLayout->addWidget(plotGroupBox);

	QScrollArea* buttonsScrollArea = new QScrollArea;
	buttonsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	buttonsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	QWidget* dummyScrollAreaWidget = new QWidget;
	dummyScrollAreaWidget->setLayout(buttonsLayout);
	buttonsScrollArea->setWidget(dummyScrollAreaWidget);
	playersAndButtonsLayout->addWidget(buttonsScrollArea);

	playersAndButtons->setLayout(playersAndButtonsLayout);
	splitter->addWidget(playersAndButtons);

	plots = new VerticalWidgetList(boost::bind(&ArenaTab::createAttributePlot, this));
	splitter->addWidget(plots);

	QVBoxLayout* dummy = new QVBoxLayout;
	dummy->addWidget(splitter);
	setLayout(dummy);

	connect(primaryPlayer, SIGNAL(drawOverlay()), this, SLOT(drawPrimaryOverlay()));
	connect(primaryPlayer, SIGNAL(frameDisplayed(size_t)), this, SLOT(primaryFrameDisplayed(size_t)));
	connect(secondaryPlayer, SIGNAL(drawOverlay()), this, SLOT(drawSecondaryOverlay()));

	connect(rightDisplayComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rightDisplayComboBoxChanged(int)));

	connect(leftDrawArrowCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(rightDrawArrowCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(leftDrawBodyCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(rightDrawBodyCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(leftDrawBocSegmentsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(leftDrawWingCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(rightDrawWingCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(leftTrailSpinBox, SIGNAL(valueChanged(double)), this, SLOT(overlaySettingsChanged()));
	connect(rightTrailSpinBox, SIGNAL(valueChanged(double)), this, SLOT(overlaySettingsChanged()));

	connect(occlusionSpinBox, SIGNAL(valueChanged(int)), this, SLOT(showOcclusion(int)));
	connect(previousOcclusionButton, SIGNAL(clicked(bool)), occlusionSpinBox, SLOT(stepDown()));
	connect(nextOcclusionButton, SIGNAL(clicked(bool)), occlusionSpinBox, SLOT(stepUp()));
	connect(switchBeforeButton, SIGNAL(clicked(bool)), this, SLOT(switchBefore()));
	connect(switchLocallyButton, SIGNAL(clicked(bool)), this, SLOT(switchLocally()));
	connect(switchAfterButton, SIGNAL(clicked(bool)), this, SLOT(switchAfter()));
	connect(saveOcclusionButton, SIGNAL(clicked(bool)), this, SLOT(saveOcclusion()));

	connect(plotSpinBox, SIGNAL(valueChanged(int)), plots, SLOT(setWidgetCount(int)));
}

ArenaTab::~ArenaTab()
{
}

QSize ArenaTab::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize ArenaTab::sizeHint() const
{
	return QSize(640, 480);
}

void ArenaTab::setProject(Project* project)
{
	primaryPlayer->setCurrentVideo(boost::shared_ptr<Video>());
	for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
		assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->reset();
	}
	currentArena = NULL;
}

void ArenaTab::setCurrentItem(Item* item)
{
	ArenaItem* arenaItem = NULL;
	if (!item || !(arenaItem = dynamic_cast<ArenaItem*>(item))) {
		primaryPlayer->setCurrentVideo(boost::shared_ptr<Video>());
		secondaryPlayer->setCurrentVideo(boost::shared_ptr<Video>());
		for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
			assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->reset();
		}
		currentArena = NULL;
		return;
	}

	currentArena = arenaItem;

	try {
		primaryPlayer->setCurrentVideo(currentArena->getVideo(), currentArena->getLeft(), currentArena->getTop(), currentArena->getWidth(), currentArena->getHeight());
		secondaryPlayer->setCurrentVideo(boost::shared_ptr<Video>(new Video(currentArena->getFileName().toStdString())), currentArena->getLeft(), currentArena->getTop(), currentArena->getWidth(), currentArena->getHeight());
	} catch (std::runtime_error& e) {
		QMessageBox::warning(this, tr("Displaying video"), e.what());
		setCurrentItem();
		return;
	}

	if ((currentArena->getCurrentVideoStage() == Item::FlyTracking && currentArena->getCurrentVideoStatus() == Item::Finished) || currentArena->getCurrentVideoStage() > Item::FlyTracking) {
		boost::shared_ptr<TrackingResults> trackingResults;
		try {
			trackingResults = currentArena->getTrackingResults();
		} catch (std::exception& e) {
			QMessageBox::warning(this, tr("Loading tracking results"), e.what());
			for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
				assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->reset();
			}
			return;
		}

		flyColors = makeColorMap(trackingResults->getFlyCount());


		for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
			assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->setCurrentResults(trackingResults);
		}

		size_t occlusionCount = trackingResults->getOcclusionMap().size();
		if (occlusionCount) {
			previousOcclusionButton->setEnabled(true);
			nextOcclusionButton->setEnabled(true);
			occlusionSpinBox->setEnabled(true);
			occlusionSpinBox->setRange(0, occlusionCount - 1);
			occlusionSpinBox->setSpecialValueText("");
		} else {
			previousOcclusionButton->setEnabled(false);
			nextOcclusionButton->setEnabled(false);
			occlusionSpinBox->setEnabled(false);
			occlusionSpinBox->setRange(0, 0);
			occlusionSpinBox->setSpecialValueText("No Occlusions");
		}

		primaryPlayer->setTimeSliderImage(currentArena->absoluteDataDirectory().filePath("ethoSlider.png"));

		currentResults = trackingResults;
	}
}

void ArenaTab::enter()
{
	if (!hasBeenEntered) {
		plotSpinBox->setValue(2);
		hasBeenEntered = true;
	}
}

void ArenaTab::leave()
{
	pause();

	if (occlusionsModified) {
		QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("MateBook"),
			tr("Occlusions have been modified.\nDo you want to save your changes?"),
			QMessageBox::Save | QMessageBox::Discard);
		if (ret == QMessageBox::Save) {
			saveOcclusion();
		}
		occlusionsModified = false;
	}
}

void ArenaTab::cut()
{
}

void ArenaTab::copy()
{
}

void ArenaTab::paste()
{
}

void ArenaTab::del()
{
}

void ArenaTab::play()
{
	primaryPlayer->play();
}

void ArenaTab::pause()
{
	primaryPlayer->pause();
}

void ArenaTab::stop()
{
	primaryPlayer->stop();
}

void ArenaTab::seek(qint64 ms)
{
	primaryPlayer->seek(ms);
}

void ArenaTab::seek(int frameNumber)
{
	primaryPlayer->seek(frameNumber);
}

void ArenaTab::rightDisplayComboBoxChanged(int index)
{
	for (int i = 1; i < rightDisplayComboBox->count(); ++i) {	// starts with i = 1 because the left player shall remain visible
		playersAndButtonsLayout->itemAt(i)->widget()->hide();
	}
	playersAndButtonsLayout->itemAt(index)->widget()->show();
}

void ArenaTab::plotCenterChanged(size_t newCenter)
{
	seek((int)newCenter);
}

void ArenaTab::plotZoomChanged(size_t newNumValues)
{
	for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
		assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->setNumValues(newNumValues);
	}
}

void ArenaTab::drawArrow(size_t frameNumber)
{
	if (currentResults && currentResults->hasData(frameNumber)) {
		for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
			glPushMatrix();
			Vf2 bodyCentroid = currentResults->getFlyData<Vf2>(flyNumber, "bodyCentroid", frameNumber);
			glTranslatef(bodyCentroid.x(), bodyCentroid.y(), 0.1);
			glRotatef(currentResults->getFlyData<float>(flyNumber, "bodyOrientation", frameNumber), 0, 0, 1);
			glScalef(currentResults->getFlyData<float>(flyNumber, "bodyMajorAxisLength", frameNumber), currentResults->getFlyData<float>(flyNumber, "bodyMinorAxisLength", frameNumber), 1);
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_DEPTH_TEST);
			glLineWidth(2);
			glColor4f(flyColors[flyNumber].x(), flyColors[flyNumber].y(), flyColors[flyNumber].z(), 0.5);
			GraphicsPrimitives::drawArrow(1, 1);
			glPopAttrib();
			glPopMatrix();
		}
	}
}

void ArenaTab::drawBody(size_t frameNumber)
{
	if (currentResults && currentResults->hasData(frameNumber)) {
		if (currentResults->getFrameData<MyBool>("isOcclusion", frameNumber)) {
			size_t contourOffset = currentResults->getFrameData<uint32_t>("bodyContourOffset", frameNumber);
			drawContour(contourOffset, QVector4D(1, 1, 1, 0.5));
			for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
				size_t contourOffset = currentResults->getFlyData<uint32_t>(flyNumber, "bodyContourOffset", frameNumber);
				drawContour(contourOffset, flyColors[flyNumber]);
			}
		} else {
			for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
				size_t contourOffset = currentResults->getFlyData<uint32_t>(flyNumber, "bodyContourOffset", frameNumber);
				drawContour(contourOffset, flyColors[flyNumber]);
			}
		}
	}
}

void ArenaTab::drawBocSegments(size_t frameNumber)
{
	if (currentResults && currentResults->hasData(frameNumber)) {
		if (currentResults->getFrameData<MyBool>("isOcclusion", frameNumber)) {
			size_t contourOffset = currentResults->getFrameData<uint32_t>("bodyContourOffset", frameNumber);
			drawContour(contourOffset, QVector4D(1, 1, 1, 0.5));
			for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
				size_t contourOffset = currentResults->getFlyData<uint32_t>(flyNumber, "bocContourOffset", frameNumber);
				drawContour(contourOffset, flyColors[flyNumber]);
			}
		} else {
			for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
				size_t contourOffset = currentResults->getFlyData<uint32_t>(flyNumber, "bodyContourOffset", frameNumber);
				drawContour(contourOffset, flyColors[flyNumber]);
			}
		}
	}
}

void ArenaTab::drawWing(size_t frameNumber)
{
	if (currentResults && currentResults->hasData(frameNumber)) {
		if (currentResults->getFrameData<MyBool>("isOcclusion", frameNumber)) {
			size_t contourOffset = currentResults->getFrameData<uint32_t>("wingContourOffset", frameNumber);
			drawContour(contourOffset, QVector4D(1, 1, 1, 0.5));
			for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
				size_t contourOffset = currentResults->getFlyData<uint32_t>(flyNumber, "wingContourOffset", frameNumber);
				drawContour(contourOffset, flyColors[flyNumber]);
			}
		} else {
			for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
				size_t contourOffset = currentResults->getFlyData<uint32_t>(flyNumber, "wingContourOffset", frameNumber);
				drawContour(contourOffset, flyColors[flyNumber]);
			}
		}
	}
}

void ArenaTab::drawContour(size_t contourFileOffset, const QVector4D& color)
{
	const std::vector<char>& contours = currentResults->getContours();

	uint32_t segmentCount = 0;
	if (contourFileOffset + sizeof(segmentCount) > contours.size()) {
		return;
	}
	segmentCount = *reinterpret_cast<const uint32_t*>(&contours[contourFileOffset]); contourFileOffset += sizeof(uint32_t);
	if (segmentCount > 10) {	// sanity check
		std::cerr << "drawContour: " << segmentCount << " segments found; sanity check failed; skipping...\n";
		return;
	}

	for (uint32_t segmentNumber = 0; segmentNumber != segmentCount; ++segmentNumber) {
		uint32_t vertexCount = 0;
		if (contourFileOffset + sizeof(vertexCount) > contours.size()) {
			return;
		}
		vertexCount = *reinterpret_cast<const uint32_t*>(&contours[contourFileOffset]); contourFileOffset += sizeof(uint32_t);
		if (contourFileOffset + vertexCount * 2 * sizeof(float) > contours.size()) {	// sanity check, *2 because we have x and y
			std::cerr << "drawContour: contour file is too small to contain " << vertexCount << " vertices; sanity check failed; skipping...\n";
			return;
		}

		//TODO: everything outside of glBegin / glEnd should be done once for the entire contour only, rather than per segment
		glPushMatrix();
		glTranslatef(0, 0, 0.05);
		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glLineWidth(2);
		glColor4f(color.x(), color.y(), color.z(), 0.5);	//TODO: use color.w() instead of 0.5?
		glBegin(GL_LINE_STRIP);
		for (uint32_t vertexNumber = 0; vertexNumber != vertexCount; ++vertexNumber) {
			float x = *reinterpret_cast<const float*>(&contours[contourFileOffset]); contourFileOffset += sizeof(float);
			float y = *reinterpret_cast<const float*>(&contours[contourFileOffset]); contourFileOffset += sizeof(float);
			glVertex2f(x, y);
		}
		glEnd();
		glPopAttrib();
		glPopMatrix();
	}
}

void ArenaTab::drawTrail(size_t frameNumber, double timeDelta)
{
	if (timeDelta && currentResults && currentResults->hasData(frameNumber)) {
		for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
			glPushMatrix();
			glTranslatef(0, 0, 0.05);
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_POINT_BIT);
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_DEPTH_TEST);
			glPointSize(10);
			glBegin(GL_POINTS);
			const float currentVideoTime = currentResults->getFrameData<float>("videoTime", frameNumber);
			for (size_t frameToDraw = frameNumber; currentResults->hasData(frameToDraw); frameToDraw += sign(timeDelta)) {
				float timeToDraw = currentResults->getFrameData<float>("videoTime", frameToDraw);
				if (abs(timeToDraw - currentVideoTime) > abs(timeDelta)) {
					break;
				}
				float normalizedTimeDelta = abs(timeToDraw - currentVideoTime) / abs(timeDelta);
				glColor4f(flyColors[flyNumber].x(), flyColors[flyNumber].y(), flyColors[flyNumber].z(), normalizedTimeDelta);
				Vf2 bodyCentroid = currentResults->getFlyData<Vf2>(flyNumber, "bodyCentroid", frameToDraw);
				glVertex2f(bodyCentroid.x(), bodyCentroid.y());
			}
			glEnd();
			glPopAttrib();
			glPopMatrix();
		}
	}
}

void ArenaTab::drawWingTip(size_t frameNumber)
{
	if (currentResults && currentResults->hasData(frameNumber)) {
		for (size_t flyNumber = 0; flyNumber != currentResults->getFlyCount(); ++flyNumber) {
			float scaleFactor = currentResults->getFlyData<float>(flyNumber, "bodyMajorAxisLength", frameNumber) / 20;

			glPushMatrix();
			Vf2 leftWingTip = currentResults->getFlyData<Vf2>(flyNumber, "leftWingTip", frameNumber);
			glTranslatef(leftWingTip.x(), leftWingTip.y(), 0.1);
			glRotatef(currentResults->getFlyData<float>(flyNumber, "bodyOrientation", frameNumber), 0, 0, 1);
			glScalef(scaleFactor, scaleFactor, 1);
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_DEPTH_TEST);
			glLineWidth(2);
			glColor4f(flyColors[flyNumber].x(), flyColors[flyNumber].y(), flyColors[flyNumber].z(), 0.5);
			GraphicsPrimitives::drawCross();
			glPopAttrib();
			glPopMatrix();

			glPushMatrix();
			Vf2 rightWingTip = currentResults->getFlyData<Vf2>(flyNumber, "rightWingTip", frameNumber);
			glTranslatef(rightWingTip.x(), rightWingTip.y(), 0.1);
			glRotatef(currentResults->getFlyData<float>(flyNumber, "bodyOrientation", frameNumber), 0, 0, 1);
			glScalef(scaleFactor, scaleFactor, 1);
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_DEPTH_TEST);
			glLineWidth(2);
			glColor4f(flyColors[flyNumber].x(), flyColors[flyNumber].y(), flyColors[flyNumber].z(), 0.5);
			GraphicsPrimitives::drawCross();
			glPopAttrib();
			glPopMatrix();
		}
	}
}

void ArenaTab::drawPrimaryOverlay()
{
	size_t frameNumber = primaryPlayer->getCurrentFrameNumber();
	if (leftDrawArrowCheckBox->isChecked()) {
		drawArrow(frameNumber);
	}
	if (leftDrawBodyCheckBox->isChecked()) {
		drawBody(frameNumber);
	}
	if (leftDrawBocSegmentsCheckBox->isChecked()) {
		drawBocSegments(frameNumber);
	}
	if (leftDrawWingCheckBox->isChecked()) {
		drawWing(frameNumber);
	}
	drawTrail(frameNumber, leftTrailSpinBox->value());
	drawWingTip(frameNumber);
}

void ArenaTab::drawSecondaryOverlay()
{
	size_t frameNumber = secondaryPlayer->getCurrentFrameNumber();
	if (rightDrawArrowCheckBox->isChecked()) {
		drawArrow(frameNumber);
	}
	if (rightDrawBodyCheckBox->isChecked()) {
		drawBody(frameNumber);
	}
	if (rightDrawWingCheckBox->isChecked()) {
		drawWing(frameNumber);
	}
	drawTrail(frameNumber, rightTrailSpinBox->value());
	drawWingTip(frameNumber);
}

void ArenaTab::overlaySettingsChanged()
{
	primaryPlayer->displayCurrentFrame();
	secondaryPlayer->displayCurrentFrame();
}

void ArenaTab::primaryFrameDisplayed(size_t frameNumber)
{
	if (currentResults && currentResults->hasData(frameNumber) && currentResults->getFrameData<MyBool>("isOcclusion", frameNumber)) {
		Occlusion currentOcclusion = currentResults->getOcclusionMap().getCurrentOcclusion(frameNumber);
		occlusionSpinBox->blockSignals(true);
		occlusionSpinBox->setValue(currentOcclusion.getNumber());
		occlusionSpinBox->blockSignals(false);
		size_t currentOcclusionEnd = currentOcclusion.getEnd();
		if (secondaryPlayer->getCurrentFrameNumber() != currentOcclusionEnd) {
			secondaryPlayer->seek(currentOcclusionEnd);
		}
	}

	//TODO: rather than clearing the grapher, set up the graphs once, remember the pointers and simply update the data
	grapher->clear();
	grapher->addUnitGraph(1000, 1.0f, true, QVector4D(0.0, 0.0, 0.0, 1.0));
	if (currentResults && currentResults->hasData(frameNumber)) {
		const float* histogram = currentResults->getSmoothHistogram(frameNumber);
		if (histogram) {
			float logHistogram[256];
			for (size_t i = 0; i != 256; ++i) {
				logHistogram[i] = std::log10(1 + histogram[i]);
			}
			grapher->addBarGraph(&logHistogram[0], (&logHistogram[0]) + 256, QVector4D(0.0, 0.0, 0.0, 1.0), 1, QVector3D(0, 0, -0.1));	//TODO: fix grapher near- and far-plane
		}

		const size_t bodyThreshold = currentResults->getFrameData<uint32_t>("bodyThreshold", frameNumber);
		const size_t wingThreshold = currentResults->getFrameData<uint32_t>("wingThreshold", frameNumber);
		const float thresholdMarkerHeight = 5;
		grapher->addBarGraph(&thresholdMarkerHeight, (&thresholdMarkerHeight) + 1, QVector4D(1.0, 0.0, 0.0, 1.0), 1, QVector3D(bodyThreshold, 0, -0.2));	//TODO: fix grapher near- and far-plane
		grapher->addBarGraph(&thresholdMarkerHeight, (&thresholdMarkerHeight) + 1, QVector4D(1.0, 0.0, 0.0, 1.0), 1, QVector3D(wingThreshold, 0, -0.3));	//TODO: fix grapher near- and far-plane
	}
}

void ArenaTab::showOcclusion(int occlusionNumber)
{
	if (currentResults && currentResults->getOcclusionMap().hasOcclusion(occlusionNumber)) {
		Occlusion occlusion = currentResults->getOcclusionMap().getOcclusion(occlusionNumber);
		size_t begin = occlusion.getBegin();
		primaryPlayer->seek(begin ? begin - 1 : begin);
		secondaryPlayer->seek(occlusion.getEnd());
	}
}

void ArenaTab::switchBefore()
{
	if (!currentResults) {
		return;
	}
	size_t occlusionNumber = occlusionSpinBox->value();
	if (currentResults->getOcclusionMap().hasOcclusion(occlusionNumber)) {
		currentResults->getOcclusionMap().switchBefore(occlusionNumber);
		// mark occlusion as inspected
		size_t begin = currentResults->getOcclusionMap().getOcclusion(occlusionNumber).getBegin();
		size_t end = currentResults->getOcclusionMap().getOcclusion(occlusionNumber).getEnd();
		for (size_t frameNumber = begin; frameNumber != end; ++frameNumber) {
			currentResults->setFrameData("occlusionInspected", frameNumber, MyBool(true));
		}
	}
	for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
		assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->attributeChanged();
	}
	primaryPlayer->displayCurrentFrame();
	secondaryPlayer->displayCurrentFrame();
	occlusionsModified = true;
}

void ArenaTab::switchLocally()
{
	if (!currentResults) {
		return;
	}
	size_t occlusionNumber = occlusionSpinBox->value();
	if (currentResults->getOcclusionMap().hasOcclusion(occlusionNumber)) {
		currentResults->getOcclusionMap().switchLocally(occlusionNumber, currentResults->getFrameData<MyBool>("occlusionInspected"), currentResults->getFrameData<float>("sCombined"), currentResults->getFrameData<float>("tCombined"));
		// mark occlusion as inspected
		size_t begin = currentResults->getOcclusionMap().getOcclusion(occlusionNumber).getBegin();
		size_t end = currentResults->getOcclusionMap().getOcclusion(occlusionNumber).getEnd();
		for (size_t frameNumber = begin; frameNumber != end; ++frameNumber) {
			currentResults->setFrameData("occlusionInspected", frameNumber, MyBool(true));
		}
	}
	for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
		assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->attributeChanged();
	}
	primaryPlayer->displayCurrentFrame();
	secondaryPlayer->displayCurrentFrame();
	occlusionsModified = true;
}

void ArenaTab::switchAfter()
{
	if (!currentResults) {
		return;
	}
	size_t occlusionNumber = occlusionSpinBox->value();
	if (currentResults->getOcclusionMap().hasOcclusion(occlusionNumber)) {
		currentResults->getOcclusionMap().switchAfter(occlusionNumber);
		// mark occlusion as inspected
		size_t begin = currentResults->getOcclusionMap().getOcclusion(occlusionNumber).getBegin();
		size_t end = currentResults->getOcclusionMap().getOcclusion(occlusionNumber).getEnd();
		for (size_t frameNumber = begin; frameNumber != end; ++frameNumber) {
			currentResults->setFrameData("occlusionInspected", frameNumber, MyBool(true));
		}
	}
	for (int plotIndex = 0; plotIndex != plots->count(); ++plotIndex) {
		assert_cast<AttributeGrapher*>(plots->getWidget(plotIndex))->attributeChanged();
	}
	primaryPlayer->displayCurrentFrame();
	secondaryPlayer->displayCurrentFrame();
	occlusionsModified = true;
}

void ArenaTab::saveOcclusion()
{
	if (!currentArena || !currentResults) {
		return;
	}

	currentResults->saveAnnotation(currentArena->getAnnotationFileName().toStdString());
	occlusionsModified = false;
	//TODO: rerun the postprocessing automatically?
}

QWidget* ArenaTab::createAttributePlot() {
	AttributeGrapher* newPlot = new AttributeGrapher;
	newPlot->setAntiAliasing(true);
	newPlot->setFontSize(20);
	if (plots->count() > 0) {
		newPlot->setCenter(assert_cast<AttributeGrapher*>(plots->getWidget(0))->getCenter());
		newPlot->setNumValues(assert_cast<AttributeGrapher*>(plots->getWidget(0))->getNumValues());
	}
	connect(primaryPlayer, SIGNAL(frameDisplayed(size_t)), newPlot, SLOT(setCenter(size_t)));
	connect(newPlot, SIGNAL(plotCenterChanged(size_t)), this, SLOT(plotCenterChanged(size_t)));
	connect(newPlot, SIGNAL(plotZoomChanged(size_t)), this, SLOT(plotZoomChanged(size_t)));
	newPlot->setCurrentResults(currentResults);
	return newPlot;
}
