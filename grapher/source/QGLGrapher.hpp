#ifndef QGLGRAPHER_HPP
#define QGLGRAPHER_HPP

#ifdef WIN32
typedef __int64 int64_t;
#endif

#include <QGLWidget>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QGLFramebufferObject>
#include <QTime>
#include <vector>
#include <set>
#include "AbstractGraph.hpp"
#include "Precision.hpp"

#include "ContinuousGraph.hpp"
#include "MarkerGraph.hpp"
#include "LineGraph.hpp"
#include "LineFillGraph.hpp"
#include "PointGraph.hpp"
#include "BarGraph.hpp"
#include "WhiskerGraph.hpp"
#include "UnitGraph.hpp"

/**
 * @class  QGLGrapher
 * @brief  class for plotting graphs
 */
class QGLGrapher : public QGLWidget
{
    Q_OBJECT
public:
	
	/**
	 *	constructor
	 *	@param	xUnit the unit of the graph in x direction
	 *	@param	parent the widgets parent
	 */
	explicit QGLGrapher(const std::string& xUnit, QWidget *parent = 0);
	
	/**
	 *	destructor
	 */
    virtual ~QGLGrapher();
	
	/**
	 *	function to resize the OpenGL window
	 *	@param	w the new width
	 *	@param	h the new height
	 */
    virtual void resizeGL( int w, int h );
	
	/**
	 *	paint event override
	 *	@param	e the QPaintEvent that triggered the function
	 */
    virtual void paintEvent( QPaintEvent *e );
	
	/**
	 *	function called when a mouse button was pressed
	 *	@param	e the corresponding mouse event
	 */
	virtual void mousePressEvent ( QMouseEvent * e );
	
	/**
	 *	function called when a mouse button was released
	 *	@param	e the corresponding mouse event
	 */
	virtual void mouseReleaseEvent ( QMouseEvent * e );
	
	/**
	 *	function called when a mouse button was double clicked
	 *	@param	e the corresponding mouse event
	 */
	virtual void mouseDoubleClickEvent ( QMouseEvent * e );
	
	/**
	 *	function called when the mouse was moved
	 *	@param	e the corresponding mouse event
	 */
	virtual void mouseMoveEvent ( QMouseEvent * e );
	
	/**
	 *	redraws the screen. it is one of the functions called by paintEvent
	 */
    virtual void redraw();
	
	/**
	 *	adds a unit graph to the grapher
	 *	@param	numValues the number of unit values
	 *	@param	spacing the frequency of the units, eg. unit every minValue + n * spacing
	 *	@param	repeat if the units have to be repeated
	 *	@param	color the color of the graph
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns a pointer to the units
	 */
	UnitGraph * addUnitGraph(size_t numValues,
			  fPoint spacing,
			  bool repeat,
			  QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
			  float lineWidth = 1,
			  QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
			  const std::string& unit = std::string()
			  );
	
	/**
	 *	function to add a line graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	color the color for all the data elements
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator>
	LineGraph * addLineGraph(
							 DataIterator dataBegin,
							 DataIterator dataEnd,
							 QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
							 float lineWidth = 1,
							 QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							 const std::string& unit = std::string()
							 )
	{
		return this->addGraph<LineGraph, DataIterator>(dataBegin, dataEnd, color, lineWidth, offset, unit);
	}
	
	/**
	 *	function to add a line graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@tparam	ColorIterator the iterator stating how to get from one color element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	colorsBegin the iterator pointing to the first color. it is assumed it can be incremented
	 *			as often as the dataBegin iterator to reach its end
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator, class ColorIterator>
	LineGraph * addLineGraph(
							 DataIterator dataBegin,
							 DataIterator dataEnd,
							 ColorIterator colorsBegin,
							 float lineWidth = 1,
							 QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							 const std::string& unit = std::string()
							 )
	{
		return this->addGraph<LineGraph, DataIterator, ColorIterator>(dataBegin, dataEnd, colorsBegin, lineWidth, offset, unit);
	}
	
	/**
	 *	function to add a line fill graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	color the color for all the data elements
	 *	@param	fillAlpha the alpha value for the filled part under the line
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator>
	LineFillGraph * addLineFillGraph(
							DataIterator dataBegin,
							DataIterator dataEnd,
							QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
							float fillAlpha = 0.5,
							float lineWidth = 1,
							QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							const std::string& unit = std::string()
							)
	{
		LineFillGraph * ndata = new LineFillGraph(dataBegin, dataEnd, color, fillAlpha, lineWidth, offset, unit);
		
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

	/**
	 *	function to add a line fill graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@tparam	ColorIterator the iterator stating how to get from one color element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	colorsBegin the iterator pointing to the first color. it is assumed it can be incremented
	 *			as often as the dataBegin iterator to reach its end
	 *	@param	fillAlpha the alpha value for the filled part under the line
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator, class ColorIterator>
	LineFillGraph * addLineFillGraph(
							DataIterator dataBegin,
							DataIterator dataEnd,
							ColorIterator colorsBegin,
							float fillAlpha = 0.5,
							float lineWidth = 1,
							QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							const std::string& unit = std::string()
							)
	{
		LineFillGraph * ndata = new LineFillGraph(dataBegin, dataEnd, colorsBegin, fillAlpha, lineWidth, offset, unit);
		
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

	/**
	 *	function to add a point graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	color the color for all the data elements
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator>
	PointGraph * addPointGraph(
							DataIterator dataBegin,
							DataIterator dataEnd,
							QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
							float lineWidth = 1,
							QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							const std::string& unit = std::string()
							)
	{
		return this->addGraph<PointGraph, DataIterator>(dataBegin, dataEnd, color, lineWidth, offset, unit);
	}

	/**
	 *	function to add a point graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@tparam	ColorIterator the iterator stating how to get from one color element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	colorsBegin the iterator pointing to the first color. it is assumed it can be incremented
	 *			as often as the dataBegin iterator to reach its end
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator, class ColorIterator>
	PointGraph * addPointGraph(
							DataIterator dataBegin,
							DataIterator dataEnd,
							ColorIterator colorsBegin,
							float lineWidth = 1,
							QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							const std::string& unit = std::string()
							)
	{
		return this->addGraph<PointGraph, DataIterator, ColorIterator>(dataBegin, dataEnd, colorsBegin, lineWidth, offset, unit);
	}
	
	/**
	 *	function to add a bar graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	color the color for all the data elements
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator>
	BarGraph * addBarGraph(
							DataIterator dataBegin,
							DataIterator dataEnd,
						   QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
						   float lineWidth = 1,
						   QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
						   const std::string& unit = std::string()
						   )
	{
		return this->addGraph<BarGraph, DataIterator>(dataBegin, dataEnd, color, lineWidth, offset, unit);
	}

	/**
	 *	function to add a bar graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@tparam	ColorIterator the iterator stating how to get from one color element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	colorsBegin the iterator pointing to the first color. it is assumed it can be incremented
	 *			as often as the dataBegin iterator to reach its end
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator, class ColorIterator>
	BarGraph * addBarGraph(
		DataIterator dataBegin,
		DataIterator dataEnd,
		ColorIterator colorsBegin,
		float lineWidth = 1,
		QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
		const std::string& unit = std::string()
	)
	{
		return this->addGraph<BarGraph, DataIterator, ColorIterator>(dataBegin, dataEnd, colorsBegin, lineWidth, offset, unit);
	}
	
	/**
	 *	function to add a whisker graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@tparam	ErrorIterator the iterator stating how to get from one error element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	errorsBegint the iterator pointing to the first error element. it is assumed it can 
	 *			be incremented as often as dataBegin to reach its end
	 *	@param	color the color for all the data elements
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator, class ErrorIterator>
	WhiskerGraph * addWhiskerGraph(
		DataIterator dataBegin,
		DataIterator dataEnd,
		ErrorIterator errorsBegin,
		QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
		float lineWidth = 1,
		QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
		const std::string& unit = std::string()
	)
	{
		WhiskerGraph * ndata = new WhiskerGraph(dataBegin, dataEnd, errorsBegin, color, lineWidth, offset, unit);
		
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

	/**
	 *	function to add a whisker graph to grapher
	 *	@tparam	DataIterator the iterator stating how to get from one data element to the next
	 *	@tparam	ErrorIterator the iterator stating how to get from one error element to the next
	 *	@tparam	ColorIterator the iterator stating how to get from one color element to the next
	 *	@param	dataBegin the iterator pointing to the first element
	 *	@param	dataEnd the end iterator
	 *	@param	errorsBegint the iterator pointing to the first error element. it is assumed it can
	 *			be incremented as often as dataBegin to reach its end
	 *	@param	colorsBegin the iterator pointing to the first color. it is assumed it can be incremented
	 *			as often as the dataBegin iterator to reach its end
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit
	 *	@return	returns a pointer to the graph
	 */
	template<class DataIterator, class ErrorIterator, class ColorIterator>
	WhiskerGraph * addWhiskerGraph(
		DataIterator dataBegin,
		DataIterator dataEnd,
		ErrorIterator errorsBegin,
		ColorIterator colorsBegin,
		float lineWidth = 1,
		QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
		const std::string& unit = std::string()
	)
	{
		WhiskerGraph * ndata = new WhiskerGraph(dataBegin, dataEnd, errorsBegin, colorsBegin, lineWidth, offset, unit);
		
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
	
	/**
	 *	function to add a marker graph to marker
	 *	@param	imageFileName the file name of the image to be used as marker
	 *	@param	x the pointer to the x value where a marker should be displayed
	 *	@param	y the pointer to the y value where a merker should be displayed
	 *	@param	count the number of markers
	 *	@param	offset the marker offset. For the sake of readability this should probably 0, since
	 *			x can be of arbitrary value compared to the other graphs
	 *	@param	scale the scale of a marker. 1.0 mans that 1 pixel corresponds to 1 texel in the image
	 *	@param	rotate the rotation of the marker, has to be given in radians
	 */
	MarkerGraph * addMarker(const std::string & imageFileName,
							const fPoint * x,
							const fPoint * y,
							size_t count,
							QVector3D offset = QVector3D(0.0, 0.0, 0.0),
							float scale = 1.0f,
							float rotate = 0.0f
	);
	
	/**
	 *	function to remove a graph
	 *	@param	graph the graph to remove
	 */
    void removeData(AbstractGraph * graph);
	
	/**
	 *	clears all the graphs from the grapher
	 */
    void clear();
	
	/**
	 *	function to respond to a mouse wheel event
	 *	@param	e the corresponding mouse event
	 */
    void wheelEvent(QWheelEvent *event);
	
	/**
	 *	saves the current image the grapher renders to an image
	 *	@param	filePath the path to the image file to be
	 */
	void saveTo(QString filePath);
	
	/**
	 *	returns the font size of all the labels in pixels
	 *	@return	int storing the font size
	 */
	int getFontSize();	// in pixels
	
	/**
	 *	returns the number of values the grapher should display
	 *	@return	size_t the number of values
	 */
	size_t getNumValues() const;
	
	/**
	 *	returns which value the grapher displays at its center
	 *	@return	size_t with the center index. can't be negative
	 */
	size_t getCenter() const;
	
	/**
	 *	returns the range of the current selection
	 *	@return returns the selection as a pair of min/max
	 */
	std::pair<int, int> getSelection() const;
	
signals:
	
	/**
	 *	signal emitted when the center has changed, e.g. when panned
	 *	@param	newCenter the new center value
	 */
	void centerChanged(size_t newCenter);	// pan
	
	/**
	 *	signal emitted when the number of values displayed has changed, e.g. when zoomed
	 *	@param	numValues the new number of values
	 */
	void numValuesChanged(size_t numValues);	// zoom
	
	/**
	 *	signal emitted when the current selection has been changed
	 *	@param	indexRange the new index range
	 */
	void currentSelectedIndexes(std::pair<int, int> indexRange);	// signal start and end index of area where mouse was dragged (right mouse button pressed)

public slots:
	
	/**
	 *	slot to center the grapher from the outside
	 *	@param	frame the index from the center frame
	 */
    void center(size_t frame);
	
	/**
	 *	slot to change the number of displayed values
	 *	@param	numValues the number of values displayed
	 */
    void setNumValues(size_t numValues);
	
	/**
	 *	slot to set the anti aliasing
	 *	@param	enable if AA is enabled
	 */
	void setAntiAliasing(bool enable);
	
	/**
	 *	slot to set the grapher to be rotated (vertical)
	 *	@param	rotated if the grapher is rotated 90 degrees
	 */
	void setRotated(bool rotated);
	
	/**
	 *	sets the font size in pixel
	 *	@param	pixel the new font size
	 */
	void setFontSize(int pixel);
	
	/**
	 *	sets if the grapher should scale depending on the set min/max value over all the graphs
	 *	@param	autoScale if auto scale is on
	 */
	void setAutoScale(bool autoScale);
	
	/**
	 *	sets if the display of the hovered value is only changed when the mouse is moved (off),
	 *	or if moving the center value of the grapher via the center slot also updates the hovered value (on)
	 *	@param	autoHover if it is on or off
	 */
	void setAutoHover(bool autoHover);
	
	/**
	 *	slot to set the selected area
	 *	@param	area the min/max pair of the selection area, seems in world units, not indices
	 */
	void setSelectedArea(std::pair<float, float> area);
	
	/**
	 *	method to clear the selection
	 */
	void clearSelection();
	
	/**
	 *	enables snapping the selection to index values
	 *	@param	snap if snap to index values is on or off
	 */
	void setSelectionSnap(bool snap);
	
private slots:
	
	/**
	 *	slot to check if the grapher should be redrawn
	 */
    void checkRedrawPending();	// slot receiving the timer event, redraws the graphs if necessary
	
private:
	
	/**
	 *	@brief	struct for defining a location along the window borders
	 *	it is used do put the text information on the screen
	 */	
	typedef enum
	{
		TOP				= 1,
		BOTTOM			= 2,
		LEFT			= 4,
		RIGHT			= 8,
		
		VCENTER			= TOP | BOTTOM,
		HCENTER			= LEFT | RIGHT,
		
		CENTER			= VCENTER | HCENTER
	}
	borderPos;
	
	/**
	 *	overridden paintGL function. triggered by redraw/paintEvent. 
	 *	does all the OpenGL calls
	 */
    virtual void paintGL();
	
	/**
	 *	initializes OpenGL
	 */
    virtual void initializeGL();
	
    /**
	 *	draws the graph at position
	 *	@param	graph the graph in the list
	 *	@param	offset the offset of the whole graph while dragging
	 *	@param	midpoint the current frame displayed at x = 0
	 */
    void drawGraph(AbstractGraph * graph,
				   int64_t offset,
				   size_t * midpoint);
	
	/**
	 *	draws the labels
	 *	@param	painter the painter to be used for drawing
	 */
	void drawLabels(QPainter & painter);
	
	/**
	 *	draws a given text at a certain position along the border
	 *	@param	m_Z the z value of the text
	 *	@param	m_Pos the position
	 *	@param	m_Text the text
	 *	@param	painter the painter to be used for drawing
	 */
	void drawText(fPoint m_Z, int m_Pos, std::string m_Text, QPainter & painter);
	
	/**
	 *	draws a given text at a certain position along the border
	 *	@param	m_Z the z value of the text
	 *	@param	m_X the position x
	 *	@param	m_Y the position y
	 *	@param	m_Text the text
	 *	@param	painter the painter to be used for drawing
	 */
	void drawTextAt(fPoint m_Z, fPoint m_X, fPoint m_Y, std::string m_Text, QPainter & painter);
	
	/**
	 *	draws a GL_QUAD between the start and end position of selectionRange_
	 */
	void drawSelectedArea();
	
	/**
	 *	calculates the hovered point
	 */
	void calculateHovered();
	
	/**
	 *	template class handling all graph types
	 *	@param	dataBegin iterator pointing to the first element
	 *	@param	dataEnd iterator pointing to the last element
	 *	@param	color the color of the graph
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns a pointer to the graph
	 */
	template<class GraphType, class DataIterator>
	GraphType * addGraph(
							 DataIterator dataBegin,
							 DataIterator dataEnd,
							 QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
							 float lineWidth = 1,
							 QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							 const std::string& unit = std::string()
							 )
	{
		GraphType * ndata = new GraphType(dataBegin, dataEnd, color, lineWidth, offset, unit);
		
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
	
	/**
	 *	template class handling all graph types with different color values
	 *	@param	dataBegin iterator pointing to the first element
	 *	@param	dataEnd iterator pointing to the last element
	 *	@param	colorsBegin iterator pointing to the first color element
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns a pointer to the graph
	 */
	template<class GraphType, class DataIterator, class ColorIterator>
	GraphType * addGraph(
							 DataIterator dataBegin,
							 DataIterator dataEnd,
							 ColorIterator colorsBegin,
							 float lineWidth = 1,
							 QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
							 const std::string& unit = std::string()
							 )
	{
		GraphType * ndata = new GraphType(dataBegin, dataEnd, colorsBegin, lineWidth, offset, unit);
		
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
	
	
	std::set<AbstractGraph *, GraphComparator>          data_;          /**< vector of the graph data elements. */
	std::string                       xUnit_;	      /**< the unit on the x axis. */
	
    size_t                            numValues_;     /**< the number of values displayed. it is always odd to have values exactly at x = 0. */
    size_t                            graphCenter_;   /**< the frame where the graph should have it's center. */
    float                             zoomFactor_;    /**< the zoom factor in vertical direction */
	QVector3D						  openGLPos_;	  /**< the position in xy space in opengl that has to be displayed. */
	
	QVector3D						  vecPosition_;   /**< the current mouse position while dragging. */
	QVector3D						  dragError_;	  /**< error vector while dragging. */
	bool							  isFirstFrame_;  /**< if the current frame is the first frame in the movement. */
	bool							  pressed_;		  /**< if the mouse button is pressed. */
	bool							  leftPressed_;	  /**< if the left button is pressed. */
	
	QGLFramebufferObject			  * imageFBO_;	  /**< the fbo to render the image into. */
	
	float							  valuedist_;	  /**< the distance between values. */
	int								  windowOffset_;  /**< the number of values the cursor offsets from the centered value when moving. */
	
	bool							  hovered_;		  /**< if we are hovering over a certain point. */
	QVector3D						  hoveredPos_;	  /**< the position of the hovered point. */
	
	size_t							  hoveredIndex_;	/**< the index of the current hovered pos. */
	AbstractGraph					  * hoveredGraph_;		/**< the hovered graph. */
	
	bool							  antialiasing_;		/**< if the curves should be rendered using antialiasing. */
	
	size_t							  lastNumDivides_;		/**< how often the units of the last frame were divided. */
	QTime							  divideOccurredTimer_;	/**< timer started/reset when a divide occurres. */
	
	bool                              rotated_;              /**< if the display is rotated. */
	
	int								  fontSize_;		/**< the font size. */
	
	fPoint							  maxValue_;		/**< the maximum value of all data. */
	fPoint							  minValue_;		/**< the minimum value of all data. */
	
	bool							  autoScale_;		/**< if the grapher should automatically scale to the min/max value. */
	
	float							  near_;			/**< the near clipping plane. */
	float							  far_;				/**< the far clipping plane. */
	
	bool                              redrawPending_;   /**< if the graphs have to be redrawn. */
	QTimer*                           redrawTimer_;     /**< causes redrawPending_ to be checked. */
	
	bool							  autoHover_;		/**< if the hovered position is calculated each frame draw. */
	
	std::pair<float, float>			  selectionRange_;	/**< selected index range */
	bool							  snapSelection_;	/**< if selection should snap to grid. */
	
	QMatrix4x4						  viewMatrix_;		/**< the view matrix. */
	QMatrix4x4						  projectionMatrix_;	/**< the projection matrix. */
	
	float							  spacing_;			/**< the spacing of the units. */
	
};

#endif // QGLGRAPHER_H
