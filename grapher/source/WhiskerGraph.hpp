/*
 *  linegraph.h
 *  grapher
 *
 *  Created by Herbert Grasberger on 04.07.11.
 *  Copyright 2011 by grasberger.org. All rights reserved.
 *
 */

#ifndef WHISKERGRAPH_HPP
#define WHISKERGRAPH_HPP

#include "ContinuousGraph.hpp"

class WhiskerGraph : public ContinuousGraph
{
public:
	
	/**
	 *	constructor
	 *	@param	data the data to be displayed
	 *	@param	errors the errorboundaries
	 *	@param	dataSize the number of values
	 *	@param	color the color of the graph
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns the id of this object
	 */
	template<class DataIterator, class ErrorIterator>
	WhiskerGraph(
		DataIterator dataBegin,
		DataIterator dataEnd,
		ErrorIterator errorsBegin,
		QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
		float lineWidth = 1,
		QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
		const std::string& unit = std::string()
	) : ContinuousGraph(dataBegin, dataEnd, color, lineWidth, offset, unit), errors_()
	{
		convertErrors(errorsBegin, dataEnd - dataBegin);
		generateIndicesAndRegenerateVertexBuffer();
		calculateExtremeValues();
	}
	
	/**
	 *	constructor
	 *	@param	data the data to be displayed
	 *	@param	errors the errorboundaries
	 *	@param	dataSize the number of values
	 *	@param	colors the color of the graph
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns the id of this object
	 */
	template<class DataIterator, class ErrorIterator, class ColorIterator>
	WhiskerGraph(
		DataIterator dataBegin,
		DataIterator dataEnd,
		ErrorIterator errorsBegin,
		ColorIterator colorsBegin,
		float lineWidth = 1,
		QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
		const std::string& unit = std::string()
	) : ContinuousGraph(dataBegin, dataEnd, colorsBegin, lineWidth, offset, unit), errors_()
	{
		convertErrors(errorsBegin, dataEnd - dataBegin);
		generateIndicesAndRegenerateVertexBuffer();
		calculateExtremeValues();
	}
	
	/**
	 *	destructor
	 */
	virtual ~WhiskerGraph();
	
	
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
	
	/**
	 *	calculates the maximum and minimum value of the data set.
	 */
	void calculateExtremeValues();

	
	/**
	 *	converts the error data to vectors
	 *	@param	errors the error boundaries
	 *	@param	dataSize the number of errors
	 */
	template<class ErrorIterator>
	void convertErrors(ErrorIterator errorsBegin, size_t dataSize)
	{
		errors_.resize(dataSize);
		for(size_t i = 0; i < dataSize; ++i)
		{
			errors_[i] = QVector3D(0.0f, errorsBegin[i], 0.0f);
		}
	}

	
	std::vector<QVector3D>		errors_;		/**< the error vectors. */
	
	
	
};

#endif //__whisker_graph_data_h
