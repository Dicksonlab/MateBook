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
#include "VideoTab.hpp"
#include "ArenaItem.hpp"
#include "RuntimeError.hpp"
#include "makeColorMap.hpp"
#include "GraphicsPrimitives.hpp"
#include "TimeDelegate.hpp"
#include "DateDelegate.hpp"
#include "GenderDelegate.hpp"
#include "FilePathDelegate.hpp"
#include "FilePathNameDelegate.hpp"
#include "ArenasApprovedDelegate.hpp"
#include "Project.hpp"
#include "ItemTree.hpp"
#include "FancyTreeView.hpp"
#include "../../common/source/debug.hpp"

VideoTab::VideoTab(QWidget* parent) : AbstractTab(parent),
	currentMode(Stop),
	currentItem(NULL),
	currentProject(NULL)
{
	QSplitter* splitter = new QSplitter;
	splitter->setOrientation(Qt::Vertical);

	QWidget* playersAndButtons = new QWidget;
	QHBoxLayout* playersAndButtonsLayout = new QHBoxLayout;
	videoPlayer = new VideoPlayer(this);
	playersAndButtonsLayout->addWidget(videoPlayer, 1);
	QVBoxLayout* buttonsLayout = new QVBoxLayout;
	buttonsLayout->setAlignment(Qt::AlignTop);

	QGroupBox* overlaysGroupBox = new QGroupBox(tr("Overlays"));
	overlaysGroupBox->setFlat(true);
	QVBoxLayout* overlaysGroupBoxLayout = new QVBoxLayout;
	drawArenaCheckBox = new QCheckBox("Arenas");
	drawArenaCheckBox->setChecked(true);
	overlaysGroupBoxLayout->addWidget(drawArenaCheckBox);
	drawEthogramCheckBox = new QCheckBox("Ethograms");
	drawEthogramCheckBox->setChecked(true);
	overlaysGroupBoxLayout->addWidget(drawEthogramCheckBox);
	overlaysGroupBox->setLayout(overlaysGroupBoxLayout);
	buttonsLayout->addWidget(overlaysGroupBox);

	QGroupBox* drawModeGroupBox = new QGroupBox(tr("Draw Mode"));
	drawModeGroupBox->setFlat(true);
	QVBoxLayout* drawModeGroupBoxLayout = new QVBoxLayout;

	drawVideoRadioButton = new QRadioButton("Video");
	drawVideoRadioButton->setChecked(true);
	drawBackgroundRadioButton = new QRadioButton("Background");
	drawDifferenceRadioButton = new QRadioButton("Difference");
	drawModeButtonGroup = new QButtonGroup;
	drawModeButtonGroup->addButton(drawVideoRadioButton, 0);
	drawModeButtonGroup->addButton(drawBackgroundRadioButton, 1);
	drawModeButtonGroup->addButton(drawDifferenceRadioButton, 2);
	drawModeGroupBoxLayout->addWidget(drawVideoRadioButton);
	drawModeGroupBoxLayout->addWidget(drawBackgroundRadioButton);
	drawModeGroupBoxLayout->addWidget(drawDifferenceRadioButton);

	drawModeGroupBox->setLayout(drawModeGroupBoxLayout);
	buttonsLayout->addWidget(drawModeGroupBox);

	QScrollArea* buttonsScrollArea = new QScrollArea;
	buttonsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	buttonsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	QWidget* dummyScrollAreaWidget = new QWidget;
	dummyScrollAreaWidget->setLayout(buttonsLayout);
	buttonsScrollArea->setWidget(dummyScrollAreaWidget);
	playersAndButtonsLayout->addWidget(buttonsScrollArea);

	playersAndButtons->setLayout(playersAndButtonsLayout);
	splitter->addWidget(playersAndButtons);

	videoView = new FancyTreeView;
	videoView->setSortingEnabled(true);
	videoView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	videoView->setSelectionBehavior(QAbstractItemView::SelectRows);
	splitter->addWidget(videoView);

	QVBoxLayout* dummy = new QVBoxLayout;
	dummy->addWidget(splitter);
	setLayout(dummy);

	connect(videoPlayer, SIGNAL(drawOverlay()), this, SLOT(drawOverlay()));
	connect(videoPlayer, SIGNAL(clicked(QPoint)), this, SLOT(clicked(QPoint)));
	connect(videoPlayer, SIGNAL(doubleClicked(QPoint)), this, SLOT(doubleClicked(QPoint)));
	connect(videoPlayer, SIGNAL(drag(QPoint, QPoint)), this, SLOT(drag(QPoint, QPoint)));
	connect(videoPlayer, SIGNAL(dragFinished(QPoint, QPoint)), this, SLOT(dragFinished(QPoint, QPoint)));
	connect(drawArenaCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(drawEthogramCheckBox, SIGNAL(stateChanged(int)), this, SLOT(overlaySettingsChanged()));
	connect(drawModeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(drawModeChanged(int)));

	// install custom delegates to be used with videoView - you should not share the same instance of a delegate between comboboxes, widget mappers or views!
	videoViewDelegates["Arenas approved"] = new ArenasApprovedDelegate(this);
	videoViewDelegates["Path"] = new FilePathDelegate(this);
	videoViewDelegates["Start"] = new TimeDelegate(this);
	videoViewDelegates["End"] = new TimeDelegate(this);
	videoViewDelegates["Date"] = new DateDelegate(this);
	videoViewDelegates["1st sex"] = new GenderDelegate(this);
	videoViewDelegates["2nd sex"] = new GenderDelegate(this);

	connect(videoView, SIGNAL(columnsWereModified()), this, SLOT(viewWasModified()));
	connect(videoView, SIGNAL(currentHasChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(currentArenaChanged()));
}

VideoTab::~VideoTab()
{
}

QSize VideoTab::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize VideoTab::sizeHint() const
{
	return QSize(640, 480);
}

void VideoTab::readHeaderSettings()
{
	if (currentProject) {
		videoView->setHeaderVisibility(currentProject->getDirectory().absolutePath() + "/videoTreeView.bin");
	}
}

void VideoTab::writeHeaderSettings()
{
	videoView->saveHeaderVisibility(currentProject->getDirectory().absolutePath() + "/videoTreeView.bin");
}

void VideoTab::setProject(Project* project)
{
	currentProject = project;
	videoPlayer->setCurrentVideo(boost::shared_ptr<Video>());
	videoView->setModel(project->getItemTree());
	//connect(videoView, SIGNAL(selectionHasChanged()), this, SLOT(updateActionAvailability()));	//TODO: implement and forward to MateBook; can we move this to the constructor after videoView->setModel()?

	// scan the headers to see if there is a custom delegate to use
	for (int columnNumber = 0; columnNumber != videoView->model()->columnCount(); ++columnNumber) {
		QString header = videoView->model()->headerData(columnNumber, Qt::Horizontal).toString();
		std::map<QString, QAbstractItemDelegate*>::const_iterator iter = videoViewDelegates.find(header);
		if (iter != videoViewDelegates.end()) {
			videoView->setItemDelegateForColumn(columnNumber, iter->second);
		}
	}

	videoView->createHeaderContextMenu(); //TODO: get and save settings in project
}

void VideoTab::setCurrentItem(Item* item)
{
	if (FileItem* fileItem = dynamic_cast<FileItem*>(item)) {
		if (currentItem == fileItem) {
			return;
		}
		currentItem = fileItem;
	} else if (ArenaItem* arenaItem = dynamic_cast<ArenaItem*>(item)) {
		fileItem = assert_cast<FileItem*>(arenaItem->parent());
		if (currentItem == fileItem) {
			return;
		}
		currentItem = fileItem;
	} else {
		videoPlayer->setCurrentVideo(boost::shared_ptr<Video>());
		videoPlayer->deleteTextures();
		currentItem = NULL;
		return;
	}

	// hide all videos from the videoView
	for (int row = 0; row < videoView->model()->rowCount(); ++row) {
		videoView->setRowHidden(row, QModelIndex(), true);
		// ...except the selected one
		if (getItem(videoView->index(row, 0)) == currentItem) {
			videoView->setRowHidden(row, QModelIndex(), false);
		}
	}
	videoView->expandAll();

	videoPlayer->deleteTextures();
	try {
		videoPlayer->setCurrentVideo(currentItem->getVideo());
	} catch (std::runtime_error& e) {
		QMessageBox::warning(this, tr("Displaying video"), e.what());
		setCurrentItem();
		return;
	}

	if (currentItem->childCount()) {
		drawArenaCheckBox->setEnabled(true);
		drawEthogramCheckBox->setEnabled(true);
		drawVideoRadioButton->setEnabled(true);
		drawBackgroundRadioButton->setEnabled(true);
		drawDifferenceRadioButton->setEnabled(true);
	} else {
		drawArenaCheckBox->setEnabled(false);
		drawEthogramCheckBox->setEnabled(false);
		drawVideoRadioButton->setEnabled(false);
		drawBackgroundRadioButton->setEnabled(false);
		drawDifferenceRadioButton->setEnabled(false);
	}
}

FancyTreeView* VideoTab::getSharedTreeView() const
{
	return videoView;
}

void VideoTab::enter()
{
}

void VideoTab::leave()
{
	pause();
}

void VideoTab::cut()
{
	/*	//TODO: see which kind of cut/copy/paste actually makes sense here
	QModelIndex currentIndex = videoView->selectionModel()->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}
	QApplication::clipboard()->setText(getCurrentProject()->data(currentIndex).toString());
	getCurrentProject()->setData(currentIndex, QVariant());
	*/
}

void VideoTab::copy()
{
	/*	//TODO: see which kind of cut/copy/paste actually makes sense here
	QModelIndex currentIndex = videoView->selectionModel()->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}
	QApplication::clipboard()->setText(getCurrentProject()->data(currentIndex).toString());
	*/
}

void VideoTab::paste()
{
	/*	//TODO: see which kind of cut/copy/paste actually makes sense here
	QModelIndex currentIndex = videoView->selectionModel()->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}
	getCurrentProject()->setData(currentIndex, QApplication::clipboard()->text());
	*/
}

void VideoTab::del()
{
	currentProject->removeItems(videoView->getSelectedIndexes());
}

void VideoTab::runArenaDetection()
{
	QModelIndexList indexes = videoView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		getItem(index)->runArenaDetection();
	}
}

void VideoTab::runFlyTracking()
{
	QModelIndexList indexes = videoView->getSelectedIndexes();
	foreach (QModelIndex index, indexes) {
		getItem(index)->runFlyTracking();
	}
}

void VideoTab::play()
{
	videoPlayer->play();
}

void VideoTab::pause()
{
	videoPlayer->pause();
}

void VideoTab::stop()
{
	videoPlayer->stop();
}

void VideoTab::seek(qint64 ms)
{
	videoPlayer->seek(ms);
}

void VideoTab::seek(int frameNumber)
{
	videoPlayer->seek(frameNumber);
}

void VideoTab::drawOverlay()
{
	if (currentItem) {
		if (drawArenaCheckBox->isChecked()) {
			// we want to draw the selected arena (if any) a little differently
			ArenaItem* selectedArena = getSelectedArenaItem();
			glPushMatrix();
			glTranslatef(0, 0, 0.1);
//			glScalef(0.5, 0.5, 1);
			glPushAttrib(GL_ENABLE_BIT);
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_TEXTURE_2D);
			for (int childNumber = 0; childNumber != currentItem->childCount(); ++childNumber) {
				ArenaItem* arenaItem = assert_cast<ArenaItem*>(currentItem->child(childNumber));
				if (arenaItem != selectedArena) {
					GraphicsPrimitives::drawQuadContour(arenaItem->getLeft(), arenaItem->getTop(), arenaItem->getWidth(), arenaItem->getHeight());
				} else {
					glPushAttrib(GL_LINE_BIT);
					glLineWidth(5);
					GraphicsPrimitives::drawQuadContour(arenaItem->getLeft(), arenaItem->getTop(), arenaItem->getWidth(), arenaItem->getHeight());
					glPopAttrib();
				}
			}
			glPopAttrib();
			glPopMatrix();
		}
		if (drawEthogramCheckBox->isChecked()) {
			glPushMatrix();
			glTranslatef(0, 0, 0.1);
//			glScalef(0.5, 0.5, 1);
			glPushAttrib(GL_ENABLE_BIT);
			for (int childNumber = 0; childNumber != currentItem->childCount(); ++childNumber) {
				ArenaItem* arenaItem = assert_cast<ArenaItem*>(currentItem->child(childNumber));

				glEnable(GL_TEXTURE_2D);
				QString fly0EthoTexFile = arenaItem->absoluteDataDirectory().filePath("0_ethoTableCell.png");
				videoPlayer->bindTexture(fly0EthoTexFile);
				GraphicsPrimitives::drawQuadTextured(arenaItem->getLeft(), arenaItem->getTop() + 18.0f / 20.0f * arenaItem->getHeight(), arenaItem->getWidth(), arenaItem->getHeight() / 20.0f);

				QString fly1EthoTexFile = arenaItem->absoluteDataDirectory().filePath("1_ethoTableCell.png");
				videoPlayer->bindTexture(fly1EthoTexFile);
				GraphicsPrimitives::drawQuadTextured(arenaItem->getLeft(), arenaItem->getTop() + 19.0f / 20.0f * arenaItem->getHeight(), arenaItem->getWidth(), arenaItem->getHeight() / 20.0f);

				// mark the current frame with a vertical line
				glDisable(GL_TEXTURE_2D);
				size_t videoFrameCount = arenaItem->getNumFrames();
				if (videoFrameCount > 0) {
					float timeRatio = 1.0f * videoPlayer->getCurrentFrameNumber() / videoFrameCount;
					GraphicsPrimitives::drawLine(
						arenaItem->getLeft() + arenaItem->getWidth() * timeRatio,
						arenaItem->getTop() + 17.5f / 20.0f * arenaItem->getHeight(),
						arenaItem->getLeft() + arenaItem->getWidth() * timeRatio,
						arenaItem->getTop() + 20.5f / 20.0f * arenaItem->getHeight()
					);
				}
			}
			glPopAttrib();
			glPopMatrix();
		}
	}
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void VideoTab::clicked(QPoint point)
{
	selectClickedArena(point);
}

void VideoTab::doubleClicked(QPoint point)
{
	if (selectClickedArena(point)) {
		emit moreDetail();
	}
}

bool VideoTab::selectClickedArena(QPoint point)
{
	if (currentItem) {
		videoView->clearSelection();
		
		for (int childNumber = 0; childNumber != currentItem->childCount(); ++childNumber) {
			ArenaItem* arenaItem = assert_cast<ArenaItem*>(currentItem->child(childNumber));
			int left = arenaItem->getLeft();
			int right = left + arenaItem->getWidth();
			
			int top = arenaItem->getTop();
			int bottom = top + arenaItem->getHeight();
			
			if ((left < point.x()) && (point.x() < right) && (top < point.y()) && (point.y() < bottom)) {
				QAbstractItemModel * model = videoView->model();
				size_t rowCount = model->rowCount();
				
				for (size_t i = 0; i < rowCount; ++i) {
					QModelIndex curIndex = model->index(i,0);
					Item* fileItem = getItem(curIndex);

					if (fileItem != currentItem) {
						continue;
					}

					size_t childCount = fileItem->childCount();
					for (size_t j = 0; j < childCount; ++j) {
						QModelIndex curArenaIndex = model->index(j, 0, curIndex);
						if (getItem(curArenaIndex) == arenaItem) {
							videoView->setCurrentIndex(curArenaIndex, QItemSelectionModel::ClearAndSelect);
							return true;
						}
					}					
				}
			}
		}
	}
	return false;
}

void VideoTab::drag(QPoint start, QPoint current)
{
}

void VideoTab::dragFinished(QPoint start, QPoint end)
{
	if (!currentItem) {
		return;
	}

	QRect boundingBox(
		std::min(start.x(), end.x()),
		std::min(start.y(), end.y()),
		std::abs(end.x() - start.x()),
		std::abs(end.y() - start.y())
	);
	currentItem->createArena(boundingBox);
}

void VideoTab::overlaySettingsChanged()
{
	videoPlayer->displayCurrentFrame();
}

void VideoTab::drawModeChanged(int radioButtonId)
{
	DrawMode drawMode = static_cast<DrawMode>(radioButtonId);

	switch (drawMode) {
		case DrawVideo:
			break;
		case DrawBackground: {
			break;
		}
		case DrawDifference: {
			break;
		}
		default:
			;
	}

	videoPlayer->displayCurrentFrame();
}

void VideoTab::currentArenaChanged()
{
	videoPlayer->displayCurrentFrame();
}

void VideoTab::toBeDeleted(FileItem* fileItem)
{
	if (currentItem == fileItem) {
		setCurrentItem();
	}
}

ArenaItem* VideoTab::getSelectedArenaItem() const
{
	QModelIndex currentIndex = videoView->currentIndex();
	if (!currentIndex.isValid() || !currentIndex.parent().isValid()) {	// checks if a child is selected or not
		return NULL;
	}
	return static_cast<ArenaItem*>(getItem(currentIndex));
}

Item* VideoTab::getItem(const QModelIndex& index) const
{
	if (!index.isValid()) {
		throw std::runtime_error("invalid index");
	}
//	return static_cast<Item*>(proxyModel->mapToSource(index).internalPointer());	//TODO: remove this if the code below works
	// here we assume that no proxy remaps column 0 and that it contains the address of the Item
	return static_cast<Item*>(index.sibling(index.row(), 0).data(Qt::UserRole).value<void*>());
}
