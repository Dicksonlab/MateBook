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
#include "VideoRenderer.hpp"
#include "Video.hpp"

VideoRenderer::VideoRenderer(QWidget *parent, QGLWidget *shareWidget) : QGLWidget(parent, shareWidget),
	currentMode(Stop),
	recording(),
	overlayDisplayList(0),
	currentVideo(),
	roiLeft(0),
	roiTop(0),
	roiWidth(1),
	roiHeight(1),
	frameData(NULL),
	lastFrameDecoded(-1),
	projectionRatio(1),
	lastPos(),
	startPos(),
	firstDragFrame(false),
	dragging(false),
	dragMode(false),
	zoomFactor(1),
	panX(0), panY(0),
	offscreenBuffer(NULL)
{
	setFocusPolicy(Qt::ClickFocus);
	setMouseTracking(true);
}

VideoRenderer::~VideoRenderer()
{
	makeCurrent();
	glDeleteTextures(bufferSize, &textures[0]);
	
	if(offscreenBuffer)
	{
		delete offscreenBuffer;
		offscreenBuffer = NULL;
	}
}

QSize VideoRenderer::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize VideoRenderer::sizeHint() const
{
	return QSize(640, 480);
}

void VideoRenderer::zoom(float delta)
{
	zoomFactor *= delta;
	zoomFactor = (zoomFactor < 0.1) ? 0.1 : zoomFactor;
	zoomFactor = (zoomFactor > 10) ? 10 : zoomFactor;
//	updateGL();
}

void VideoRenderer::pan(float dX, float dY)
{
	panX += dX;
	panY += dY;
//	updateGL();
}

void VideoRenderer::setCurrentVideo(boost::shared_ptr<Video> video)
{
	frameData = NULL;
	lastFrameDecoded = -1;

	if (!video) {
		// null pointer passed, so unset the current video
		currentVideo = video;
		return;
	}

	// sanity checks
	if (video->getWidth() <= 0 || video->getHeight() <= 0 || video->getFps() <= 0 || video->getNumFrames() <= 0) {
		throw std::runtime_error("video meta data didn't pass sanity checks");
	}
//	if (frameDataInRAM.size() < video->getWidth() * video->getHeight() * 3) {
//		frameDataInRAM.resize(video->getWidth() * video->getHeight() * 3);
//	}
	roiLeft = 0;
	roiTop = 0;
	roiWidth = video->getWidth();
	roiHeight = video->getHeight();
	currentVideo = video;

	this->makeCurrent();
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, currentVideo->getWidth());
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, roiLeft);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, roiTop);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)roiWidth, (int)roiHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glPopClientAttrib();
	
	if(offscreenBuffer)
	{
		delete offscreenBuffer;
		offscreenBuffer = NULL;
	}
	offscreenBuffer = new QGLFramebufferObject(roiWidth, roiHeight);/*,
											   QGLFramebufferObject::CombinedDepthStencil,
											   GL_TEXTURE_2D, GL_RGB);*/
}

void VideoRenderer::setCurrentVideo(boost::shared_ptr<Video> video, unsigned int roiLeft, unsigned int roiTop, unsigned int roiWidth, unsigned int roiHeight)
{
	frameData = NULL;
	lastFrameDecoded = -1;

	if (!video) {
		// null pointer passed, so unset the current video
		currentVideo = video;
		return;
	}

	// sanity checks
	if (video->getWidth() <= 0 || video->getHeight() <= 0 || video->getFps() <= 0 || video->getNumFrames() <= 0) {
		throw std::runtime_error("video meta data didn't pass sanity checks");
	}
	if (video->getWidth() < roiLeft + roiWidth || video->getHeight() < roiTop + roiHeight) {
		throw std::runtime_error("region of interest is not contained within the video");
	}
	if (roiWidth == 0 || roiHeight == 0) {
		throw std::runtime_error("region of interest width and height must be positive");
	}
//	if (frameDataInRAM.size() < video->getWidth() * video->getHeight() * 3) {
//		frameDataInRAM.resize(video->getWidth() * video->getHeight() * 3);
//	}
	this->roiLeft = roiLeft;
	this->roiTop = roiTop;
	this->roiWidth = roiWidth;
	this->roiHeight = roiHeight;
	currentVideo = video;

	this->makeCurrent();
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, currentVideo->getWidth());
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, roiLeft);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, roiTop);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)roiWidth, (int)roiHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glPopClientAttrib();
	
	if(offscreenBuffer)
	{
		delete offscreenBuffer;
		offscreenBuffer = NULL;
	}
	offscreenBuffer = new QGLFramebufferObject(roiWidth, roiHeight);/*,
											   QGLFramebufferObject::CombinedDepthStencil,
											   GL_TEXTURE_2D, GL_RGB);*/
}

void VideoRenderer::startRecording(const QString& fileName)
{
	recording.reset(new mw::OutputVideo(const_cast<char*>(fileName.toStdString().c_str()), roiWidth, roiHeight, currentVideo->getFps()));
	overlayDisplayList = glGenLists(1);
}

void VideoRenderer::stopRecording()
{
	recording->close();
	recording.reset();
	glDeleteLists(overlayDisplayList, 1);
	overlayDisplayList = 0;
}

void VideoRenderer::bindExtraTexture(const QString& fileName)
{
	makeCurrent();
	std::map<QString, GLuint>::const_iterator iter = extraTextures.find(fileName);
	if (iter != extraTextures.end()) {
		glBindTexture(GL_TEXTURE_2D, iter->second);
	} else {
		QImage cpuImage(fileName);
		GLuint handle = bindTexture(cpuImage, GL_TEXTURE_2D, GL_RGBA, QGLContext::NoBindOption);
		extraTextures[fileName] = handle;
	}
}

void VideoRenderer::deleteExtraTextures()
{
	makeCurrent();
	for (std::map<QString, GLuint>::const_iterator iter = extraTextures.begin(); iter != extraTextures.end(); ++iter) {
		deleteTexture(iter->second);
	}
	extraTextures.clear();
}

void VideoRenderer::initializeGL()
{
/*
	cerr << "gl vendor: " << glGetString(GL_VENDOR) << endl;
	cerr << "gl renderer: " << glGetString(GL_RENDERER) << endl;
	cerr << "gl version: " << glGetString(GL_VERSION) << endl;
	cerr << "gl extensions: " << endl;
	istringstream extensionStream(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
	string extension;
	while (extensionStream >> extension) {
		cerr << "  " << extension << endl;
	}
	cerr << "glu version: " << gluGetString(GLU_VERSION) << endl;
	cerr << "glu extensions: " << endl;
	istringstream gluExtensionStream(reinterpret_cast<const char*>(gluGetString(GLU_EXTENSIONS)));
	string gluExtension;
	while (gluExtensionStream >> gluExtension) {
		cerr << "  " << gluExtension << endl;
	}
*/
#if defined(WIN32)
	std::cerr << "calling glewInit()" << std::endl;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		std::cerr << "GLEW error: " << glewGetErrorString(glewError) << std::endl;
	}
#endif

	qglClearColor(Qt::black);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glGenTextures(bufferSize, &textures[0]);
	for (size_t i = 0; i < bufferSize; ++i) {
		glBindTexture(GL_TEXTURE_2D, textures[i]);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAPS, GL_FALSE);
	}
}

void VideoRenderer::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	
	if(offscreenBuffer)
		offscreenBuffer->bind();
	
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, roiWidth, roiHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	float videoRatio = 1.0 * roiWidth / roiHeight;
	if (videoRatio > 1) {	// landscape
		glOrtho(-videoRatio, videoRatio, -1, 1, -1, 1);
	} else {	// portrait
		glOrtho(-1, 1, -1 / videoRatio, 1 / videoRatio, -1, 1);
	}
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	renderToTexture();
	
	glPopMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopAttrib();
	
	if(offscreenBuffer)
		offscreenBuffer->release();
	
	glTranslatef(panX, panY, 0);
	glScalef(zoomFactor, zoomFactor, zoomFactor);
	drawVideoFrame();
	
	if(dragMode)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		QPoint videoCoordinate = lastPos;
		
		if(videoCoordinate.x() < 0)
		{
			videoCoordinate.setX(0);
		}
		if(videoCoordinate.x() > (int)roiWidth)
		{
			videoCoordinate.setX(roiWidth);
		}
		
		if(videoCoordinate.y() < 0)
		{
			videoCoordinate.setY(0);
		}
		
		if(videoCoordinate.y() > (int)roiHeight)
		{
			videoCoordinate.setY(roiHeight);
		}
		
		glColor3f(1.0f, 1.0f, 1.0f);
		
		glBegin(GL_LINES);
		
		glVertex3f(videoCoordinate.x(), 0, 0.1f);
		glVertex3f(videoCoordinate.x(), roiHeight, 0.1f);
		
		glVertex3f(0, videoCoordinate.y(), 0.1f);
		glVertex3f(roiWidth, videoCoordinate.y(), 0.1f);
		
		glEnd();
		
		if(!firstDragFrame && dragging)
		{
			glBegin(GL_LINE_LOOP);
			
			glVertex3f(startPos.x(), startPos.y(), 0.1f);
			glVertex3f(videoCoordinate.x(), startPos.y(), 0.1f);
			glVertex3f(videoCoordinate.x(), videoCoordinate.y(), 0.1f);
			glVertex3f(startPos.x(), videoCoordinate.y(), 0.1f);
			
			glEnd();
		}
		
	}
	
	if (recording) {
//		glNewList(overlayDisplayList, GL_COMPILE_AND_EXECUTE);
	}
	emit bufferAboutToBeSwapped();
	if (recording && offscreenBuffer) {
				
		if(offscreenBuffer)
			offscreenBuffer->bind();
		
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0, 0, roiWidth, roiHeight);
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		
		glLoadIdentity();
		float videoRatio = 1.0 * roiWidth / roiHeight;
		if (videoRatio > 1) {	// landscape
			glOrtho(-videoRatio, videoRatio, -1, 1, -1, 1);
		} else {	// portrait
			glOrtho(-1, 1, -1 / videoRatio, 1 / videoRatio, -1, 1);
		}
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		float scaleFactor = 2.0 / (int)roiHeight;
		glScalef(scaleFactor, scaleFactor, 1);
		glTranslatef(-(int)roiWidth / 2.0, -(int)roiHeight / 2.0, 0);
		
//		glTranslatef(0, (int)roiHeight, 0);
//		glScalef(1, -1, 1);
		
		emit bufferAboutToBeSwapped();
		
		glPopMatrix();
		
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		
		glMatrixMode(GL_MODELVIEW);
		
		glPopAttrib();
		
		if(offscreenBuffer)
			offscreenBuffer->release();
		
//		glEndList();
		QImage curFrame = QGLWidget::convertToGLFormat(offscreenBuffer->toImage());
//		std::cout << curFrame.width() << " " << curFrame.height() << " " << curFrame.format() << std::endl;
		recording->appendVideoFrame(curFrame.bits(), PIX_FMT_RGBA); // 2 for PIX_FMT_RGB24
	}
}

void VideoRenderer::resizeGL(int width, int height)
{
	if (width <= 0) {
		width = 1;
	}

	if (height <= 0) {
		height = 1;
	}

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	projectionRatio = 1.0 * width / height;
	if (projectionRatio > 1) {	// landscape
		glOrtho(-projectionRatio, projectionRatio, -1, 1, -1, 1);
	} else {	// portrait
		glOrtho(-1, 1, -1 / projectionRatio, 1 / projectionRatio, -1, 1);
	}
	glMatrixMode(GL_MODELVIEW);
}

void VideoRenderer::mousePressEvent(QMouseEvent* event)
{
	lastPos = convertToVideoCoordinates(event->pos());
	
	if((event->buttons() & Qt::LeftButton) && (event->modifiers() & Qt::ShiftModifier))
	{
		dragging = true;
		firstDragFrame = true;
	}
	event->accept();
}

void VideoRenderer::mouseMoveEvent(QMouseEvent* event)
{
	QPoint curPos = convertToVideoCoordinates(event->pos());
	if(dragging)
	{
		if(firstDragFrame)
		{
			startPos = curPos;
			firstDragFrame = false;
			return;
		}
		else
		{
			emit drag(startPos, curPos);
		}
		setCursor(Qt::CrossCursor);
	}
	else
	{
//		const float mouseSensitivity = 1.0 / 80;		
		
//		float dx = mouseSensitivity * (event->x() - lastPos.x());
//		float dy = mouseSensitivity * (event->y() - lastPos.y());

//		if (event->buttons() & Qt::LeftButton) {
//			zoom(dy);
//		} else 
		if (event->buttons() & Qt::RightButton) {
			this->moveImage(lastPos, curPos);
			curPos = convertToVideoCoordinates(event->pos());
		}
	}
	lastPos = curPos;//event->pos();
	event->accept();
	if(dragging || dragMode || event->buttons() & Qt::RightButton)
	{
		this->updateGL();
	}
}

void VideoRenderer::mouseReleaseEvent(QMouseEvent* event)
{
	if(dragging)
	{
		dragging = false;
		emit dragFinished(startPos, convertToVideoCoordinates(event->pos()));
	}
	else 
	{
		emit clicked(convertToVideoCoordinates(event->pos()));
	}
	event->accept();
	this->updateGL();
}

void VideoRenderer::mouseDoubleClickEvent(QMouseEvent* event)
{
	emit doubleClicked(convertToVideoCoordinates(event->pos()));
	event->accept();
}

void VideoRenderer::keyPressEvent(QKeyEvent *event)
{
	if(event->modifiers() & Qt::ShiftModifier)
	{
		dragMode = true;
		setCursor(Qt::CrossCursor);
	}
	else
	{
		QGLWidget::keyPressEvent(event);
	}
	this->updateGL();
}

void VideoRenderer::keyReleaseEvent(QKeyEvent * event)
{
	if(!(event->modifiers() & Qt::ShiftModifier))
	{
		dragMode = false;
		setCursor(Qt::ArrowCursor);
		
		if(dragging)
		{
			dragging = false;
			emit dragFinished(startPos, convertToVideoCoordinates(lastPos));
		}
	}
	else
	{
		QGLWidget::keyPressEvent(event);
	}
	this->updateGL();
}

void VideoRenderer::wheelEvent(QWheelEvent *event)
{
	if (event) {
		QPoint firstPos = convertToVideoCoordinates(event->pos());
		
		zoom((event->delta() < 0) ? 1.25 : 0.8);
		
		QPoint secondPos = convertToVideoCoordinates(event->pos());
		
		this->moveImage(firstPos, secondPos);
		lastPos = convertToVideoCoordinates(event->pos());
		
		event->accept();
	}
	event->ignore();
	this->updateGL();
}

QPoint VideoRenderer::convertToVideoCoordinates(QPoint windowPoint)
{
	// convert to normalized OpenGL [-1, 1] range
	float glWidth = (((float)windowPoint.x() / (float)width()) - 0.5f) * 2.0f;
	float glHeight = (((float)(/*height() - */windowPoint.y()) / (float)height()) - 0.5f) * 2.0f;
	
	if (projectionRatio > 1) {	// landscape
		glWidth *= projectionRatio;
	} else {	// portrait
		glHeight *= (1.0f / projectionRatio);
	}
	
	QPoint localCoordinates;
	
	float videoRatio = 1.0 * (int)roiWidth / (int)roiHeight;
	float scaleFactor = 1.0;
	if (videoRatio > projectionRatio) { // black stripes at the top and bottom
		if (projectionRatio > 1) {
			scaleFactor = 2.0 * projectionRatio / (int)roiWidth;
		} else {
			scaleFactor = 2.0 / (int)roiWidth;
		}
	} else { // black stripes left and right
		if (projectionRatio < 1) {
			scaleFactor = 2.0 / (projectionRatio * (int)roiHeight);
		} else {
			scaleFactor = 2.0 / (int)roiHeight;
		}
	}
	
	float glOriginX = (-(int)roiWidth / 2.0f) * scaleFactor * zoomFactor + panX;
	float glOriginY = (-(int)roiHeight / 2.0f) * scaleFactor * zoomFactor - panY;
	
	glWidth = glWidth - glOriginX;
	glHeight = glHeight - glOriginY;
	
//	glWidth -= panX;
//	glHeight -= panY;
	
	glWidth /= scaleFactor * zoomFactor;
	glHeight /= scaleFactor * zoomFactor;

	
	localCoordinates.setX((int)glWidth);
	localCoordinates.setY((int)glHeight);
	
	return localCoordinates;
}

void VideoRenderer::drawVideoFrame()
{
	if (!currentVideo) {
		return;
	}
	if (offscreenBuffer) {
		glBindTexture(GL_TEXTURE_2D, offscreenBuffer->texture());

		// make best use of the available screenspace, while still displaying an undistorted video
		float videoRatio = 1.0 * (int)roiWidth / (int)roiHeight;
		float scaleFactor = 1.0;
		if (videoRatio > projectionRatio) { // black stripes at the top and bottom
			if (projectionRatio > 1) {
				scaleFactor = 2.0 * projectionRatio / (int)roiWidth;
			} else {
				scaleFactor = 2.0 / (int)roiWidth;
			}
		} else { // black stripes left and right
			if (projectionRatio < 1) {
				scaleFactor = 2.0 / (projectionRatio * (int)roiHeight);
			} else {
				scaleFactor = 2.0 / (int)roiHeight;
			}
		}

		glScalef(scaleFactor, scaleFactor, 1);
		glTranslatef(-(int)roiWidth / 2.0, -(int)roiHeight / 2.0, 0);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 1);
			glVertex3f(0, 0, 0);
			glTexCoord2f(1, 1);
			glVertex3f((int)roiWidth, 0, 0);
			glTexCoord2f(1, 0);
			glVertex3f((int)roiWidth, (int)roiHeight, 0);
			glTexCoord2f(0, 0);
			glVertex3f(0, (int)roiHeight, 0);
		glEnd();

		glTranslatef(0, (int)roiHeight, 0);
		glScalef(1, -1, 1);
	}
}

void VideoRenderer::renderToTexture()
{
	if (!currentVideo) {
		return;
	}
	if (currentVideo->getCurrentFrameNumber() != lastFrameDecoded) {
		frameData = currentVideo->getCurrentFrame();
		lastFrameDecoded = currentVideo->getCurrentFrameNumber();
		//		std::cout << "last frame decoded " << lastFrameDecoded << std::endl;
		if (frameData) {
			glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, currentVideo->getWidth());
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, roiLeft);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, roiTop);
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (int)roiWidth, (int)roiHeight, GL_RGB, GL_UNSIGNED_BYTE, frameData);
			glPopClientAttrib();
		}
	}
	if (frameData) {
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		
		float scaleFactor = 2.0 / (int)roiHeight;
		
		glScalef(scaleFactor, scaleFactor, 1);
		glTranslatef(-(int)roiWidth / 2.0, -(int)roiHeight / 2.0, 0);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex3f(0, 0, 0);
		glTexCoord2f(1, 0);
		glVertex3f((int)roiWidth, 0, 0);
		glTexCoord2f(1, 1);
		glVertex3f((int)roiWidth, (int)roiHeight, 0);
		glTexCoord2f(0, 1);
		glVertex3f(0, (int)roiHeight, 0);
		glEnd();
		
		glTranslatef(0, (int)roiHeight, 0);
		glScalef(1, -1, 1);
	}
}

void VideoRenderer::moveImage(QPoint from, QPoint to)
{
	QPoint delta = to - from;
	
	float videoRatio = 1.0 * (int)roiWidth / (int)roiHeight;
	float scaleFactor = 1.0;
	if (videoRatio > projectionRatio) { // black stripes at the top and bottom
		if (projectionRatio > 1) {
			scaleFactor = 2.0 * projectionRatio / (int)roiWidth;
		} else {
			scaleFactor = 2.0 / (int)roiWidth;
		}
	} else { // black stripes left and right
		if (projectionRatio < 1) {
			scaleFactor = 2.0 / (projectionRatio * (int)roiHeight);
		} else {
			scaleFactor = 2.0 / (int)roiHeight;
		}
	}
	
	float dx = (float)delta.x() * (scaleFactor * zoomFactor);
	float dy = (float)delta.y() * (scaleFactor * zoomFactor);
	
	pan(dx, -dy);
}
