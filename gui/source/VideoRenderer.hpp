#ifndef VideoRenderer_hpp
#define VideoRenderer_hpp

#include <QGLWidget>
#include <QGLFramebufferObject>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "../../mediawrapper/source/mediawrapper.hpp"

class Video;

class VideoRenderer : public QGLWidget
{
	Q_OBJECT

public:
	enum Mode { Play, Pause, Stop };

	VideoRenderer(QWidget *parent = 0, QGLWidget *shareWidget = 0);
	~VideoRenderer();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void zoom(float delta);
	void pan(float dX, float dY);

	void setCurrentVideo(boost::shared_ptr<Video> video);
	void setCurrentVideo(boost::shared_ptr<Video> video, unsigned int roiLeft, unsigned int roiTop, unsigned int roiWidth, unsigned int roiHeight);

	void startRecording(const QString& fileName);
	void stopRecording();

	void bindExtraTexture(const QString& fileName);
	void deleteExtraTextures();

signals:
	void clicked(QPoint);
	void doubleClicked(QPoint);
	void drag(QPoint start, QPoint current);
	void dragFinished(QPoint start, QPoint end);
	void bufferAboutToBeSwapped();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void wheelEvent(QWheelEvent *event);
	QPoint convertToVideoCoordinates(QPoint windowPoint);

private:
	void drawVideoFrame();
	
	void renderToTexture();
	
	void moveImage(QPoint from, QPoint to);

	Mode currentMode;
	boost::shared_ptr<mw::OutputVideo> recording;
	unsigned int overlayDisplayList;
	boost::shared_ptr<Video> currentVideo;
	unsigned int roiLeft;
	unsigned int roiTop;
	unsigned int roiWidth;
	unsigned int roiHeight;

	unsigned char* frameData;	// memory owned by mediawrapper
	int lastFrameDecoded;

	float projectionRatio; // width / height of the space projected to the viewport

	QPoint lastPos;
	QPoint startPos;
	bool firstDragFrame;
	bool dragging;
	bool dragMode;
	float zoomFactor;
	float panX, panY;

	static const size_t bufferSize = 2;
	GLuint textures[bufferSize];

	std::map<QString, GLuint> extraTextures;	// textures added via bindTexture
	
	QGLFramebufferObject	* offscreenBuffer;
};

#endif
