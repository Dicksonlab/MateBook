#if defined(WIN32) || defined(LINUX)
  #include <GL/glu.h>
  #include <GL/gl.h>
#else
  #include <QtOpenGL>
#endif

#include "QGLGrapher.hpp"


#include <sstream>

#include <iostream>
#include <math.h>
#include <limits>

#include <QWheelEvent>
#include <QTimer>
#include <cmath>

#include "pow.hpp"
#include "../../common/source/mathematics.hpp"

QGLGrapher::QGLGrapher(const std::string& xUnit, QWidget *parent) :
QGLWidget(QGLFormat(
					QGL::DoubleBuffer | QGL::DepthBuffer
#if !defined(WIN32)
					| QGL::AlphaChannel
#endif
					| QGL::SampleBuffers)
		  , parent),
data_(),
xUnit_(xUnit),
numValues_(501),
graphCenter_(0),
zoomFactor_(1.0f),
openGLPos_(),
vecPosition_(),
dragError_(),
isFirstFrame_(false),
pressed_(false),
leftPressed_(false),
imageFBO_(NULL),
valuedist_(0.1f),
windowOffset_(0),
hovered_(false),
hoveredPos_(),
hoveredGraph_(NULL),
antialiasing_(false),
lastNumDivides_(0),
divideOccurredTimer_(),
rotated_(false),
fontSize_(72),
maxValue_(-std::numeric_limits<fPoint>::infinity()),
minValue_(std::numeric_limits<fPoint>::infinity()),
autoScale_(false),
near_(0.0),
far_(0.0),
redrawPending_(true),
redrawTimer_(),
autoHover_(true),
snapSelection_(false),
viewMatrix_(),
projectionMatrix_(),
spacing_(0.0f)
{
	selectionRange_.first = 0;
	selectionRange_.second = 0;
	
	valuedist_ = 1.0f / ((numValues_ - 1) / 2);
	hoveredIndex_ = 0;
	
	this->setMouseTracking(true);
	divideOccurredTimer_.start();
	
	redrawTimer_ = new QTimer(this);
	connect(redrawTimer_, SIGNAL(timeout()), this, SLOT(checkRedrawPending()));
	redrawTimer_->start(30);
	
	setAutoFillBackground(false);
}

QGLGrapher::~QGLGrapher()
{
    clear();
}

void QGLGrapher::resizeGL(int w, int h)
{
    QWidget::resize(w, h);
	
	if(w == 0 || h == 0)
	{
		return;
	}
	
	projectionMatrix_.setToIdentity();
	projectionMatrix_.ortho(-1.0, 1.0, -1.0, 1.0, near_, far_);
	
    //Set projection matrix
    glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(projectionMatrix_.data());
	
    //Return to ModelView Matrix
    glMatrixMode(GL_MODELVIEW);
	
	
    glViewport(0, 0, (GLsizei) this->width(), (GLsizei) this->height());
	
	if(imageFBO_)
	{
		delete imageFBO_;
	}
	
	QGLFramebufferObjectFormat form;
	form.setSamples(16);
	
	imageFBO_ = new QGLFramebufferObject(w, h, form);
	
}

void QGLGrapher::paintEvent(QPaintEvent *e)
{
    //QGLWidget::paintEvent(e);
	
	makeCurrent();
	paintGL();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	/**
	 *	we draw the labels with a separate paint function,
	 *	so they are properly present in the exported image
	 */
	QPainter painter(this);
	
	drawLabels(painter);
	
	painter.end();
}

void QGLGrapher::mousePressEvent ( QMouseEvent * e )
{
	vecPosition_ = QVector3D( (2.0f * float(e->x()) - float(width()))  / float(width()),
							 ( float(height()) - 2.0f * float(e->y())) / float(height()),
							 0.0f);
	dragError_ = QVector3D();
	
	setCursor(QCursor(Qt::ClosedHandCursor));
	isFirstFrame_ = true;
	
	if(e->button() == Qt::LeftButton)
	{
		selectionRange_.first = 0; // unselect selected area
		selectionRange_.second = 0;
		
		leftPressed_ = true;
	}
	else if(e->button() == Qt::RightButton)
	{
		pressed_ = true;
	}
};

void QGLGrapher::mouseReleaseEvent ( QMouseEvent * e )
{
	isFirstFrame_ = true;
	setCursor(QCursor(Qt::ArrowCursor));
	
	if(e->button() == Qt::LeftButton)
	{
		leftPressed_ = false;
	}
	else if(e->button() == Qt::RightButton)
	{
		pressed_ = false;
		
		// emit selected area as signal (contains index range of selected area of the current displayed window)
		emit currentSelectedIndexes(selectionRange_);
	}
};

void QGLGrapher::mouseDoubleClickEvent(QMouseEvent* e)
{
	QVector3D v( (2.0f * float(e->x()) - float(width()))  / float(width()),
				( float(height()) - 2.0f * float(e->y())) / float(height()),
				0.0f);
	
	if (rotated_) {
		float tmp = v.x();
		v.setX(v.y());
		v.setY(-tmp);
	}
	
	QVector3D tmp;
	if (rotated_) {
		tmp.setX(-v.y());
		tmp.setY(v.x());
	} else {
		tmp.setX(v.x());
		tmp.setY(v.y());
	}
	
	bool invertible;
	/**
	 *	calculating the point in worldspace under the mouse to be the new grapher center
	 */
	QMatrix4x4 viewprojection = (projectionMatrix_ * viewMatrix_).inverted(&invertible);
	QVector3D worldPos = viewprojection * tmp;
	
	center(worldPos.x());
	emit centerChanged(graphCenter_);
};


void QGLGrapher::mouseMoveEvent ( QMouseEvent * e )
{
	// v is the position of the mouse
	// cursor over canvas: v ranges in R, from -1 to 1 in both directions, left bottom corner is -1,-1
	// cursor n.o. canvas: if mouse was pressed before cursor left canvas, position is outside the range R
	QVector3D v( (2.0f * float(e->x()) - float(width()))  / float(width()),
				( float(height()) - 2.0f * float(e->y())) / float(height()),
				0.0f);
	if(rotated_)
	{
		float tmp = v.x();
		v.setX(v.y());
		v.setY(-tmp);
	}
	
	
	if (pressed_ || leftPressed_)
	{
		if(isFirstFrame_)
		{
			vecPosition_ = v;
			isFirstFrame_ = false;
			redrawPending_ = true;
			
			/**
			 *	right mouse button, selection
			 *	we figure out the first selected value
			 */
			if(pressed_){
				
				bool invertable;
				QMatrix4x4 viewprojection = (projectionMatrix_ * viewMatrix_).inverted(& invertable);
				
				QVector3D tmp;
				if(rotated_)
				{
					tmp.setX(-v.y());
					tmp.setY(v.x());
				}
				else
				{
					
					tmp.setX(v.x());
					tmp.setY(v.y());
				}
				
				
				QVector3D worldPos = viewprojection * tmp;
				
				selectionRange_.first = worldPos.x();
				if(snapSelection_)
				{
					selectionRange_.first = round(selectionRange_.first);
				}
			}
			
			return;
		}
		
		/**
		 *	some other frame than the other of mouse interaction
		 */
		QVector3D vecPosition = v;
		QVector3D vecDifference = vecPosition - vecPosition_;
		//std::cout << vecDifference.x() << " " << vecDifference.y() << " " << vecDifference.z() << std::endl;
		//std::cout << openGLPos_.x() << " ";
		if(pressed_)
		{
			/**
			 *	updated the second value for the selection
			 */
			bool invertable;
			QMatrix4x4 viewprojection = (projectionMatrix_ * viewMatrix_).inverted(& invertable);
			
			QVector3D tmp;
			if(rotated_)
			{
				tmp.setX(-v.y());
				tmp.setY(v.x());
			}
			else
			{
				
				tmp.setX(v.x());
				tmp.setY(v.y());
			}
			
			
			QVector3D worldPos = viewprojection * tmp;
			selectionRange_.second = worldPos.x();
			
			if(snapSelection_)
			{
				selectionRange_.second = round(selectionRange_.second);
			}
			// openGLPos_ += vecDifference;
		}
		else if(leftPressed_)
		{
			/**
			 *	dragging the graph
			 */
			float totaldifference;
			if(vecDifference.x() < 0)
			{
				totaldifference = vecDifference.x() - dragError_.x();
			}
			else
			{
				totaldifference = vecDifference.x() + dragError_.x();
			}
			
			
			/**
			 *	we drag in integer values, not continuous to get a "snap to" value effect
			 */
			if(fabs(totaldifference) > valuedist_)
			{
				int64_t change = ((int64_t) (-totaldifference / valuedist_));
				int64_t tempCenter = graphCenter_ + change;
				graphCenter_ += change;
				if(tempCenter < 0)
				{
					graphCenter_ = 0;
				}
				
				double diff = fabs(totaldifference) - fabs(valuedist_ * change);
				
				/**
				 *	keeping track of the drag error when only doing integers in a float world
				 */
				dragError_.setX(diff);
				
				vecPosition_ = v;
				
				emit centerChanged(graphCenter_);
			}
		}
		
		//std::cout << openGLPos_.x() << " " << vecDifference.x() << std::endl;
	}
	
	if(!leftPressed_)
	{
		vecPosition_ = v;
	}
	
	if(pressed_ || leftPressed_)
	{
		hovered_ = false;
		redrawPending_ = true;
		return;
	}
	
	calculateHovered();
	
	
	//std::cout << "hovered " << hovered_ << " index " << divisor << " difference " << difference << std::endl;
	redrawPending_ = true;
};

void QGLGrapher::redraw()
{
    this->makeCurrent();
    this->paintGL();
	
	
	QPainter painter(this);
	painter.beginNativePainting();
	
	drawLabels(painter);
	
	painter.endNativePainting();
	
    this->swapBuffers();
	
}

UnitGraph * QGLGrapher::addUnitGraph(size_t numValues,
									 fPoint spacing,
									 bool repeat,
									 QVector4D color/* = QVector4D(0.0, 0.0, 0.0, 1.0)*/,
									 float lineWidth/* = 1*/,
									 QVector3D offset/* = QVector3D(0.0, 0.0, 0.0)*/,
									 const std::string& unit/* = std::string()*/
									 )
{
	UnitGraph * ndata = new UnitGraph(numValues, spacing, repeat, color, lineWidth, offset, unit);
	
	data_.insert(ndata);
	
	spacing_ = spacing;
	
	redrawPending_ = true;
	
	return ndata;
}

MarkerGraph * QGLGrapher::addMarker(const std::string & imageFileName,
									const fPoint * x,
									const fPoint * y,
									size_t count,
									QVector3D offset/* = QVector3D(0.0, 0.0, 0.0)*/,
									float scale/* = 1.0f*/,
									float rotate/* = 0.0f*/)
{
	QImage cpuImage(imageFileName.c_str());
	
	MarkerGraph * ndata = new MarkerGraph(cpuImage, this, cpuImage.width(), cpuImage.height(), x, y, count, offset, scale, rotate);
	data_.insert(ndata);

	
	if(ndata->getMaxValue() > maxValue_)
	{
		maxValue_ = ndata->getMaxValue();
	}
	if(ndata->getMinValue() < minValue_)
	{
		minValue_ = ndata->getMinValue();
	}
	
	redrawPending_ = true;
	
	return ndata;
}

void QGLGrapher::removeData(AbstractGraph * graph)
{
	std::set<AbstractGraph *, GraphComparator>::iterator iter = data_.find(graph);
	if (iter == data_.end()) {
		throw std::runtime_error("Attempted to remove a graph that did not exist.");
	}
	data_.erase(graph);
	delete graph;
	
	minValue_ = 0;
	maxValue_ = 0;
	
	std::set<AbstractGraph *, GraphComparator>::iterator i = data_.begin();
	while(i != data_.end())
	{
		if((*i)->getMaxValue() > maxValue_)
		{
			maxValue_ = (*i)->getMaxValue();
		}
		if((*i)->getMinValue() < minValue_)
		{
			minValue_ = (*i)->getMinValue();
		}
		++i;
	}
	
	redrawPending_ = true;
}

void QGLGrapher::clear()
{
	std::set<AbstractGraph *, GraphComparator>::iterator i = data_.begin();
	while(i != data_.end())
	{
		delete *i;
		++i;
	}
	data_.clear();
	minValue_ = 0;
	maxValue_ = 0;
	near_ = 0.0f;
	far_ = 0.0f;
	
	redrawPending_ = true;
}

void QGLGrapher::wheelEvent(QWheelEvent* event)
{
	if (event->modifiers() & Qt::ShiftModifier) {	// vertical zoom
		zoomFactor_ *= (event->delta() > 0) ? 1.25 : 0.8;
		if (zoomFactor_ < 0.1f) {
			zoomFactor_ = 0.1f;
		}
		event->accept();
		redrawPending_ = true;
		return;
	}
	
	setNumValues(numValues_ * ((event->delta() > 0) ? 1.25 : 0.8));
	event->accept();
}

void QGLGrapher::saveTo(QString filePath)
{
	this->makeCurrent();
	imageFBO_->bind();
	paintGL();
	imageFBO_->release();
	
	/**
	 *	labels need to be dealt with separate from normal OpenGL rendering,
	 *	since we use Qts overpainting for it, thus rendering text like standard Qt
	 */
	QImage tempImage = imageFBO_->toImage();
	
	
	QPainter painter(& tempImage);
	painter.beginNativePainting();
	
	drawLabels(painter);
	
	painter.endNativePainting();
	
	tempImage.save(filePath);
}

int QGLGrapher::getFontSize()
{
	return fontSize_;
}

size_t QGLGrapher::getNumValues() const
{
	return numValues_;
}

size_t QGLGrapher::getCenter() const
{
	return graphCenter_;
}

std::pair<int, int> QGLGrapher::getSelection() const
{
	return selectionRange_;
}

void QGLGrapher::center(size_t frame)
{
    graphCenter_ = frame;
	selectionRange_.first = 0; // unselect selected area
	selectionRange_.second = 0;
    
    redrawPending_ = true;
}

void QGLGrapher::setNumValues(size_t numValues)
{
	if(numValues % 2 == 0)
	{
		++numValues;
	}
	if(numValues < 3)
	{
		numValues = 3;
	}
	
	valuedist_ = 1.0f / ((numValues - 1) / 2);
	float factor = 1.0 / numValues_ * numValues;
	
	if (numValues_ != numValues) {	// only emit if there's a change (this is to break signal loops)
		numValues_ = numValues;
		// change selected area according to zoom factor
		emit numValuesChanged(numValues);
	}
	redrawPending_ = true;
}

void QGLGrapher::setAntiAliasing(bool enable)
{
	antialiasing_ = enable;
	
	redrawPending_ = true;
}

void QGLGrapher::setRotated(bool rotated)
{
	rotated_ = rotated;
	
	redrawPending_ = true;
}

void QGLGrapher::setFontSize(int pixel)
{
	fontSize_ = pixel;
	
	redrawPending_ = true;
}

void QGLGrapher::setAutoScale(bool autoScale)
{
	autoScale_ = autoScale;
	
	redrawPending_ = true;
}

void QGLGrapher::setAutoHover(bool autoHover)
{
	autoHover_ = autoHover;
	
	redrawPending_ = true;
}

void QGLGrapher::setSelectedArea(std::pair<float, float> area)
{
	selectionRange_ = area;
	redrawPending_ = true;
}

void QGLGrapher::clearSelection()
{
	setSelectedArea(std::make_pair(0, 0));
}

void QGLGrapher::setSelectionSnap(bool snap)
{
	snapSelection_ = snap;
}

void QGLGrapher::checkRedrawPending()
{
	if (redrawPending_) {
		redrawPending_ = false;
		update();
	}
}

void QGLGrapher::paintGL()
{
	if(autoHover_)
		calculateHovered();
	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if(antialiasing_)
		glEnable(GL_MULTISAMPLE);
	
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	/**
	 *	calculating the view and projection matrices based on the information 
	 *	of centering, values displayed and the set data
	 */
	viewMatrix_.setToIdentity();
	projectionMatrix_.setToIdentity();
	
	if(!data_.empty())
	{
		near_ = -(*data_.begin())->getOffset().z();
		far_ = -(*(--data_.end()))->getOffset().z();
	}
	
	float stepSign = ((far_ - near_) >= 0.0f) ? 1.0f : -1.0f;
	near_ -= 2.0f * stepSign;
	far_ += 2.0f * stepSign;
	
	if (rotated_) {
		viewMatrix_.lookAt(QVector3D((float)graphCenter_, 0.0, 0.0), QVector3D((float)graphCenter_, 0.0, -1.0), QVector3D(1.0, 0.0, 0.0));
		
		if (autoScale_) {
			projectionMatrix_.ortho(-(float)maxValue_ * zoomFactor_, -(float)minValue_ * zoomFactor_, -(float)((numValues_ - 1) / 2), +(float)((numValues_ - 1) / 2), near_, far_);
		} else {
			projectionMatrix_.ortho(-(float)maxValue_ * zoomFactor_, +(float)maxValue_ * zoomFactor_, -(float)((numValues_ - 1) / 2), +(float)((numValues_ - 1) / 2), near_, far_);
		}
	} else {
		viewMatrix_.lookAt(QVector3D((float)graphCenter_, 0.0, 0.0), QVector3D((float)graphCenter_, 0.0, -1.0), QVector3D(0.0, 1.0, 0.0));
		
		if (autoScale_) {
			projectionMatrix_.ortho(-(float)((numValues_ - 1) / 2), +(float)((numValues_ - 1) / 2), (float)minValue_ * zoomFactor_, (float)maxValue_ * zoomFactor_, near_, far_);
		} else {
			projectionMatrix_.ortho(-(float)((numValues_ - 1) / 2), +(float)((numValues_ - 1) / 2), -(float)maxValue_ * zoomFactor_, +(float)maxValue_ * zoomFactor_, near_, far_);
		}
	}
	
	
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(projectionMatrix_.data());
	
	glPushMatrix();
	
	
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(viewMatrix_.data());
	
	
	float framex = 0.0;
	size_t midpoint;
	
	int64_t indexoffset = openGLPos_.x() / valuedist_;
	
	/**
	 *	draw the selection
	 */
    this->drawSelectedArea();
	
	glPushMatrix();
	
	glDisable(GL_DEPTH_TEST);
	
	/**
	 *	drawing every graph
	 */
	std::set<AbstractGraph *, GraphComparator>::iterator i = data_.begin();
	while(i != data_.end())
	{
		this->drawGraph(*i, indexoffset, & midpoint);
		
		++i;
	}
	
	glEnable(GL_DEPTH_TEST);
	
	glPopMatrix();
	
	float zValue = 0.0f;
	
	fPoint high = maxValue_;
	fPoint low = -maxValue_;
	
	if(autoScale_)
		low = minValue_ ;
	
	
	/**
	 * drawing the center line
	 */
	glBegin(GL_LINES);
	glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(graphCenter_, high, zValue);
    glVertex3f(graphCenter_, low, zValue);
    glEnd();
	
	glColor3f(0.0f, 0.0f, 0.0f);
	
	/**
	 *	drawing the hovered point, if there is one
	 */
	if(hovered_)
	{
		glPushAttrib(GL_POINT_BIT);
		glPointSize(10.0f);
		glBegin(GL_POINTS);
		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3f(hoveredPos_.x(), hoveredPos_.y(), zValue);
		glEnd();
		glPopAttrib();
		
		glBegin(GL_LINES);
		glVertex3f(hoveredPos_.x(), high, zValue);
		glVertex3f(hoveredPos_.x(), low, zValue);
		glEnd();
	}
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	
	
	if(antialiasing_)
		glDisable(GL_MULTISAMPLE);
}

void QGLGrapher::initializeGL()
{
    this->makeCurrent();
	
#if defined(WIN32)
	std::cerr << "calling glewInit()" << std::endl;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		std::cerr << "GLEW error: " << glewGetErrorString(glewError) << std::endl;
	}
#endif
	
    glClearColor( 1.0, 1.0, 1.0, 1.0 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_POINT_SMOOTH);
	glDisable(GL_MULTISAMPLE);
}

void QGLGrapher::drawGraph(AbstractGraph * graph, int64_t offset, size_t * midpoint)
{
	fPoint high = maxValue_;
	fPoint low = -maxValue_;
	
	if(autoScale_)
		low = minValue_ ;
	
	fPoint leftMost = -(fPoint)((numValues_ - 1) / 2) + (fPoint)graphCenter_;
	fPoint rightMost = (fPoint)((numValues_ - 1) / 2) + (fPoint)graphCenter_;
	
	graph->draw(viewMatrix_, projectionMatrix_, width(), height(), high, low, leftMost, rightMost, rotated_, zoomFactor_);
}

void QGLGrapher::drawLabels(QPainter & painter)
{
	/**
	 *	all labels are displayed at z = 0, so they are on top of the graph
	 */
	float zValue = 0.0f;
	
	fPoint high = maxValue_;
	fPoint low = -maxValue_;
	
	if(autoScale_)
		low = minValue_ ;	
	
	painter.setPen(Qt::black);
	
	/**
	 *	when hovered, the units displayed at the y axis are from the hovered graph
	 */
	if(hoveredGraph_)
	{
		char cText[1024];
		sprintf(cText, "max: %.4f %s", hoveredGraph_->getMaxValue(), hoveredGraph_->getUnits().c_str());
		
		drawText(zValue, TOP | HCENTER, std::string(cText), painter);
	}
	
	
	fPoint leftMost = -(fPoint)((numValues_ - 1) / 2) + (fPoint)graphCenter_;
	fPoint rightMost = (fPoint)((numValues_ - 1) / 2) + (fPoint)graphCenter_;
	
	int64_t valuesDisplayed = rightMost - leftMost;
	
	
	if(hovered_)
	{
		
		float modifier[2];
		modifier[0] = 0.01f * valuesDisplayed;
		modifier[1] = 0.01f * (high - low);
		
		/**
		 *	drawing the value what we just hovered next to the cursor
		 */
		if(hoveredGraph_)
		{
			painter.setPen(Qt::blue);
			if(hoveredGraph_->getDataPtr() && (hoveredIndex_ < hoveredGraph_->getSize()))
			{
				char cText[1024];
				sprintf(cText, "%.4f", hoveredGraph_->getDataPtr()[hoveredIndex_].y());
				
				drawTextAt(zValue, hoveredPos_.x() + modifier[0], hoveredPos_.y() + modifier[1], std::string(cText), painter);
			}
		}
	}
	
	painter.setPen(Qt::black);
	
	if(spacing_ > 0)
	{
		
		/**
		 *	adding the units to our unit graph, based on the levels that are actually displayed
		 */
		int level = (int)((float)log((float)(valuesDisplayed - 1) / 2.0f)) / log(spacing_ * 10.0f);
		
		//		if(level > 0)
		{
			level = pow((spacing_ * 10.0f), (float)level); // if spacing is > 0 then the numbers are displayed at a factor of 10 more
			
			
			float startUnit = ((int)(leftMost / level) - 1) * level;
			float endUnit = startUnit + 2.0f * level;
			
			if(startUnit < leftMost && endUnit > rightMost)
			{
				level /= (spacing_ * 10.0f);
			}
			
			QMatrix4x4 pseudoView;
			pseudoView.lookAt(QVector3D((float)graphCenter_, 0.0, 0.0), QVector3D((float)graphCenter_, 0.0, -1.0), QVector3D(0.0, 1.0, 0.0));
			
			for(; startUnit < (int)rightMost; startUnit += level)
			{
				
				char cText[1024];
				sprintf(cText, "%.0f", startUnit);
				
				std::string text(cText);
				
				double x = graphCenter_;
				double y = 0.0;
				
				bool invertable;
				QMatrix4x4 viewprojectionInverse = (projectionMatrix_ * pseudoView).inverted(& invertable);
				
				/**
				 *	calculating the size of the text, so we can move it away from the Widget border
				 *	properly
				 */
				QFontMetrics metric(font());
				
				QString qText(text.c_str());
				
				float w = metric.width(qText);
				float h = metric.height();
				
				QRect size = metric.boundingRect(qText);
				
				double length;
				
				QVector4D mSize(w / width() * 2, h / height() * 2, 0.0, 0.0);
				QVector4D vSize = viewprojectionInverse * mSize;
				
				length = vSize.x();
				double horizontalBorder = (high - low)/ height();//height();
				
				if(rotated_)
				{
					y += low + length;// - (horizontalBorder + length);
				}
				else
				{
					y += low;
					y += horizontalBorder;
				}
				
				/**
				 *	transforming the text position into opengl coordinates, to properly place the text
				 */				
				QVector4D coordinate((float)startUnit, y, zValue, 1.0f);
				QVector4D transformed = projectionMatrix_  * viewMatrix_ * coordinate;
				transformed /= transformed.w();
				transformed *= QVector4D(0.5f, 0.5f, 1.0f, 1.0f);
				transformed += QVector4D(0.5f, 0.5f, 0.0f, 0.0f);
				
				int ppx = transformed.x() * width();
				int ppy = height()* (1.0f - transformed.y()) - 5;
				
				painter.drawText(ppx, ppy, qText);
			}
		}
		
	}
}

void QGLGrapher::drawText(fPoint m_Z, int m_Pos, std::string m_Text, QPainter & painter)
{
	/**
	 *	calculating the OpenGL position of the text, based on the text size in pixels, 
	 *	which is back transformed into OpenGL coordinates
	 */
	fPoint high = maxValue_;
	fPoint low = -maxValue_;
	
	if(autoScale_)
		low = minValue_ ;
	
	fPoint leftMost = -(fPoint)((numValues_ - 1) / 2);
	fPoint rightMost = (fPoint)((numValues_ - 1) / 2);
	
	double baseSize = 1.0;
    double horizontalBorder = (high - low)/ height();
	double verticalBorder = (rightMost - leftMost) / width();
	
	QMatrix4x4 pseudoView;
	pseudoView.lookAt(QVector3D((float)graphCenter_, 0.0, 0.0), QVector3D((float)graphCenter_, 0.0, -1.0), QVector3D(0.0, 1.0, 0.0));
	
	bool invertable;
	QMatrix4x4 viewprojectionInverse = (projectionMatrix_ * pseudoView).inverted(& invertable);
	
	QFontMetrics metric(font());
	
	QString qText(m_Text.c_str());
	
	float w = metric.width(qText);
	float h = metric.height();
	
	QRect size = metric.boundingRect(qText);
	
	QVector4D mSize(w / width() * 2, h / height() * 2, 0.0, 0.0);
	QVector4D vSize = viewprojectionInverse * mSize;
	
	
	
	double fontSize = vSize.y();
	
	double length = vSize.x();
	
	/**
	 *	text size in OpenGL done
	 */
	
	double x = graphCenter_;
	double y = 0.0;
	
	
	/**
	 *	placing the text along the widget border
	 */
	if(m_Pos & TOP)
	{
		if(rotated_)
		{
			y += high;// - (horizontalBorder);
		}
		else
		{
			y += high - (horizontalBorder + fontSize);
		}
	}
	if(m_Pos & BOTTOM)
	{
		if(rotated_)
		{
			y += low + length;// - (horizontalBorder + length);
		}
		else
		{
			y += low - (horizontalBorder);
		}
	}
	
	if(m_Pos & LEFT)
	{
		if(rotated_)
		{
			x -= -leftMost - (verticalBorder);
		}
		else
		{
			x -= -leftMost - verticalBorder;
		}
	}
	if(m_Pos & RIGHT)
	{
		if(rotated_)
		{
			x += rightMost - (verticalBorder + fontSize);
		}
		else
		{
			x += rightMost - (verticalBorder + length);
		}
	}
	
	/**
	 *	converting border position to the actual text start position
	 */
	QVector4D coordinate(x, y, m_Z, 1.0f);
	QVector4D transformed = projectionMatrix_  * viewMatrix_ * coordinate;
	transformed /= transformed.w();
	transformed *= QVector4D(0.5f, 0.5f, 1.0f, 1.0f);
	transformed += QVector4D(0.5f, 0.5f, 0.0f, 0.0f);
	
	int ppx = transformed.x() * width();
	int ppy = height()* (1.0f - transformed.y()) - 5;
	
	painter.drawText(ppx, ppy, qText);
}

void QGLGrapher::drawTextAt(fPoint m_Z, fPoint m_X, fPoint m_Y, std::string m_Text, QPainter & painter)
{
	/**
	 *	calculating the OpenGL position of the text, based on the text size in pixels,
	 *	which is back transformed into OpenGL coordinates
	 */
	fPoint high = maxValue_;
	fPoint low = -maxValue_;
	
	if(autoScale_)
		low = minValue_ ;
	
	fPoint leftMost = -(fPoint)((numValues_ - 1) / 2) + (fPoint)graphCenter_;
	fPoint rightMost = (fPoint)((numValues_ - 1) / 2) + (fPoint)graphCenter_;
	
	QMatrix4x4 pseudoView;
	pseudoView.lookAt(QVector3D((float)graphCenter_, 0.0, 0.0), QVector3D((float)graphCenter_, 0.0, -1.0), QVector3D(0.0, 1.0, 0.0));
	
	bool invertable;
	QMatrix4x4 viewprojectionInverse = (projectionMatrix_ * pseudoView).inverted(& invertable);
	
	QFontMetrics metric(font());
	
	QString qText(m_Text.c_str());
	
	float w = metric.width(qText);
	float h = metric.height();
	
	QRect size = metric.boundingRect(qText);
	
	/**
	 *	text size in OpenGL done
	 */
	
	QVector4D mSize(w / width() * 2, h / height() * 2, 0.0, 0.0);
	if(rotated_)
	{
		mSize.setX(w / height() * 2);
		mSize.setY(h / width() * 2);
	}
	QVector4D vSize = viewprojectionInverse * mSize;
	
	if((!rotated_ && (m_X + vSize.x()) > rightMost))
	{
		m_X -= vSize.x();
	}
	
	if((rotated_ && (m_X + vSize.y()) > rightMost))
	{
		m_X -= vSize.y();
	}
	
	if((!rotated_ && (m_Y + vSize.y()) > high))
	{
		m_Y -= vSize.y();
	}
	
	if(rotated_ && (m_Y - vSize.x()) < low)
	{
		m_Y += vSize.x();
	}
	
	/**
	 *	converting given position to the actual text start position
	 */
	QVector4D coordinate(m_X, m_Y, m_Z, 1.0f);
	QVector4D transformed = projectionMatrix_  * viewMatrix_ * coordinate;
	transformed /= transformed.w();
	transformed *= QVector4D(0.5f, 0.5f, 1.0f, 1.0f);
	transformed += QVector4D(0.5f, 0.5f, 0.0f, 0.0f);
	
	int ppx = transformed.x() * width();
	int ppy = height()* (1.0f - transformed.y()) - 5;
	
	painter.drawText(ppx, ppy, qText);
}

void QGLGrapher::drawSelectedArea()
{
	if(!data_.empty())
	{
		float zValue = (*(--data_.end()))->getOffset().z();
		fPoint high = maxValue_;
		fPoint low = -maxValue_;
		
		if(autoScale_)
			low = minValue_ ;
		
		glColor4f(0.7764f, 0.88627f, 1.0f, 1.0f);
		glBegin(GL_QUADS);
		glVertex3f(selectionRange_.first, low,  zValue);	// Bottom Left
		glVertex3f(selectionRange_.second, low,  zValue);	// Bottom Right
		glVertex3f(selectionRange_.second,  high,  zValue);	// Top Right
		glVertex3f(selectionRange_.first,  high,  zValue);	// Top Left
		glEnd();
	}
}

void QGLGrapher::calculateHovered()
{
	bool invertable;
	QMatrix4x4 viewprojection = (projectionMatrix_ * viewMatrix_).inverted(& invertable);
	
	QVector3D tmp;
	if(rotated_)
	{
		tmp.setX(-vecPosition_.y());
		tmp.setY(vecPosition_.x());
	}
	else
	{
		
		tmp.setX(vecPosition_.x());
		tmp.setY(vecPosition_.y());
	}
	
	/**
	 *	hovered pos in [-1,1] range to actual world pos
	 */
	QVector3D worldPos = viewprojection * tmp;
	
	/**
	 *	calculating approximate index of values, so we don't
	 *	have to compare against the whole data arrays
	 */
	int64_t index = floor(worldPos.x() + 0.5);
	
	hoveredPos_.setX((float) index);
	hoveredPos_.setY(0.0f);
	
	float difference = std::numeric_limits<float>::max();
	
	hoveredGraph_ = NULL;
	hovered_ = false;
	
	/**
	 *	finding the closest point to the world position
	 */
	std::set<AbstractGraph *, GraphComparator>::iterator i = data_.begin();
	while(i != data_.end())
	{
		ContinuousGraph * data = dynamic_cast<ContinuousGraph * > (*i);
		if(data && (data->getSize() > (index - data->getOffset().x())))
		{
			int64_t realIndex = index - data->getOffset().x();
			if(realIndex < 0)
			{
				++i;
				continue;
			}
			
			float curValue = (data->getDataPtr()[realIndex].y());
			
			QVector3D dataPos(hoveredPos_.x(), curValue, 0.0);
			QVector3D distanceVec = worldPos - dataPos;
			
			float distance = distanceVec.length();
			/**
			 *	if distance is closer, than previous distance to point
			 */
			if(distance < difference)
			{
				difference = distance;
				hoveredPos_.setY(curValue);
				
				hoveredIndex_ = realIndex;
				hovered_ = true;
				hoveredGraph_= data;
			}
		}
		++i;
	}
}
