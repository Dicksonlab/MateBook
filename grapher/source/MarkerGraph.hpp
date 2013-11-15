/*
 *  markergraph.h
 *  grapher
 *
 *  Created by Herbert Grasberger on 21.11.10.
 *  Copyright 2010 by grasberger.org. All rights reserved.
 *
 */

#ifndef MARKERGRAPH_HPP
#define MARKERGRAPH_HPP

#include "AbstractGraph.hpp"
#include <QImage>

#include <vector>

class QGLGrapher;

/**
 *	@brief	struct storing the necessary image information
 */
typedef struct
{
	unsigned int handle_;		/**< the opengl handle. */
	unsigned int width_;		/**< the width of the image. */
	unsigned int height_;		/**< the height of the image. */
}
imageInfo;

class MarkerGraph : public AbstractGraph
{
public:
	
	/**
	 *	constructor with given values
	 *	@param	image the cpu side image
	 *	@param	context the context to be used to bind the texture
	 *	@param	imageWidth the width of the texture in px
	 *	@param	imageHeight the height of the image in px
	 *	@param	x the x values where the markers should be placed w/o zooming
	 *	@param	y the y values where the markers should be placed w/o zooming
	 *	@param	count the number of markers
	 *	@param	offset the offset of the graph
	 *	@param	scale the scale of the markers, 1 = 1pixel == 1 texel
	 *	@param	rotate the rotation of the markers in rads
	 */
	MarkerGraph(QImage image,
				QGLGrapher * context,
				size_t imageWidth,
				size_t imageHeight,
			   const fPoint* x,
			   const fPoint* y,
			   size_t count,
				QVector3D offset = QVector3D(0.0, 0.0, 0.0),
			   float scale = 1.0f,
			   float rotate = 0.0f);
	
	/**
	 *	default destructor
	 */
	virtual ~MarkerGraph();
	
    /**
	 *	draws the graph at position
	 *	@param	view the view matrix
	 *	@param	projection the projection matrix
	 *	@param	width the width of the screen
	 *	@param	height the height of the screen
	 *	@param	maxValue the maximum value of all currently displayed
	 *	@param	minValue the minimum value of all currently displayed
	 *	@param	leftValue the leftmost value
	 *	@param	rightValue the rightmost value
	 *	@param	rotated if the display is rotated 90°
	 *	@param	zoomFactor the factor the data values are zoomed
	 */
    virtual void draw(QMatrix4x4 & view,
					  QMatrix4x4 & projection,
					  size_t width,
					  size_t height,
					  fPoint maxValue,
					  fPoint minValue,
					  fPoint leftValue,
					  fPoint rightValue,
					  bool rotated,
					  float zoomFactor) ;
	
	/**
	 *	gets the data pointer
	 *	@return	returns pointer to the first object
	 */
	virtual const QVector3D * getDataPtr() const;
	
	/**
	 *	gets the max value of the dataset
	 *	@return	returns the max value
	 */
	virtual fPoint getMaxValue() const;
	
	/**
	 *	gets the minimum value of the dataset
	 *	@return	returns the min value
	 */
	virtual fPoint getMinValue() const;
	
	/**
	 *	gets the units
	 *	@return	returns the unit string
	 */
	virtual std::string getUnits() const;
	
	/**
	 *	gets the number of values
	 *	@return	returns the number of values
	 */
	virtual size_t getSize() const;
	
private:
	
	QImage				baseImage_;		/**< the cpu image data. */
	
	imageInfo			markerInfo_;	/**< the information of the marker texture. */
	const fPoint		* x_;			/**< the x values of the markers. */
	const fPoint		* y_;			/**< the y values of the markers. */
	std::vector<fPoint>	copiedX_;		/**< a copy of the x values. */
	std::vector<fPoint>	copiedY_;		/**< a copy of the y values. */
	size_t				count_;			/**< the number of total markers. */
	float				scale_;			/**< the marker scale. */
	float				rotate_;		/**< the marker rotation. */
	
	fPoint					maxValue_;			/**< the maximum value in the data set. */
	fPoint					minValue_;			/**< the minimum value in the data set. */
	
};


#endif // MARKERGRAPH_HPP
