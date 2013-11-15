/*
 *  abstractgraph.h
 *  grapher
 *
 *  Created by Herbert Grasberger on 15.11.10.
 *  Copyright 2010 by grasberger.org. All rights reserved.
 *
 */

#ifndef ABSTRACTGRAPH_HPP
#define ABSTRACTGRAPH_HPP

#ifdef WIN32
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif
#include <string>

#include <QVector3D>

#include "precision.hpp"

/**
 *	@class	AbstractGraph
 *	@brief	Abstract class representing graph data
 */
class AbstractGraph
{	
public:	
	
	/**
	 *	constructor
	 *	@param	offset the offset in OpenGL units
	 */
	AbstractGraph(QVector3D offset);
	
	/**
	 *	destructor
	 */
	virtual ~AbstractGraph();
	
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
					  float zoomFactor) = 0;

	/**
	 *	gets the data pointer
	 *	@return	returns pointer to the first object
	 */
	virtual const QVector3D * getDataPtr() const = 0;
	
	/**
	 *	gets the max value of the dataset
	 *	@return	returns the max value
	 */
	virtual fPoint getMaxValue() const = 0;
	
	/**
	 *	gets the minimum value of the dataset
	 *	@return	returns the min value
	 */
	virtual fPoint getMinValue() const = 0;
	
	/**
	 *	gets the units
	 *	@return	returns the unit string
	 */
	virtual std::string getUnits() const = 0;
	
	/**
	 *	gets the number of values
	 *	@return	returns the number of values
	 */
	virtual size_t getSize() const = 0;
	
	/**
	 *	gets the index offset
	 *	@return	returns the offset
	 */
	virtual QVector3D getOffset() const;
	
protected:
	
	QVector3D				offset_;			/**< the offset for drawing. */
	
};

/**
 *	@class	GraphComparator
 *	@brief	functor class for sorting AbstractGraph objects
 */
class GraphComparator
{
public:
	
	bool operator()(const AbstractGraph * g1, const AbstractGraph * g2) const
	{
		QVector3D o1 = g1->getOffset();
		QVector3D o2 = g2->getOffset();
		if (o1.z() == o2.z()) {
			return g1 < g2;	// so we can have multiple graphs at the same depth
		}
		return (o1.z() < o2.z());
	}
};

#endif // ABSTRACTGRAPH_HPP
