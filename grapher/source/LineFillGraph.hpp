/*
 *  linegraph.h
 *  grapher
 *
 *  Created by Herbert Grasberger on 04.07.11.
 *  Copyright 2011 by grasberger.org. All rights reserved.
 *
 */

#ifndef LINEFILLGRAPH_HPP
#define LINEFILLGRAPH_HPP

#include "ContinuousGraph.hpp"

class LineFillGraph : public ContinuousGraph
{
public:
	
	/**
	 *	constructor
	 *	@param	data the data to be displayed
	 *	@param	dataSize the number of values
	 *	@param	color the color of the graph
	 *	@param	fillAlpha the fill alpha
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns the id of this object
	 */
	template<class DataIterator>
	LineFillGraph(
		DataIterator dataBegin,
		DataIterator dataEnd,
		QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
		float fillAlpha = 0.5,
		float lineWidth = 1,
		QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
		const std::string& unit = std::string()
		) :
	ContinuousGraph(dataBegin, dataEnd, color, lineWidth, offset, unit), fillAlpha_(fillAlpha)
	{
		generateIndicesAndRegenerateVertexBuffer();
	}

	
	/**
	 *	constructor with given values
	 *	@param	data the data to be displayed
	 *	@param	dataSize the number of values
	 *	@param	colors the individual colors of values
	 *	@param	fillAlpha the fill alpha
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 	
	 */
	template<class DataIterator, class ColorIterator>
	LineFillGraph(
		DataIterator dataBegin,
		DataIterator dataEnd,
		ColorIterator colorsBegin,
		float fillAlpha = 0.5,
		float lineWidth = 1,
		QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
		const std::string& unit = std::string()
		) :
	ContinuousGraph(dataBegin, dataEnd, colorsBegin, lineWidth, offset, unit), fillAlpha_(fillAlpha)
	{
		generateIndicesAndRegenerateVertexBuffer();
	}
	
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
	 *	generates the index buffer for the given primitivetype.
	 *	this method also regenerates the vertexbuffer with the added data
	 */
	virtual void generateIndicesAndRegenerateVertexBuffer();

private:

	
	float			fillAlpha_;		/**< the alpha value of the fill. */
	
};

#endif //LINEFILLGRAPH_HPP
