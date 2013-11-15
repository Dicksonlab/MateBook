/*
 *  unitgraph.h
 *  grapher
 *
 *  Created by Herbert Grasberger on 12.08.11.
 *  Copyright 2011 by grasberger.org. All rights reserved.
 *
 */

#ifndef UNITGRAPH_HPP
#define UNITGRAPH_HPP

#include "ContinuousGraph.hpp"

/**
 *	@class	UnitGraph
 *	@brief	class providing the functionality to draw units including lods
 *	units are drawn in the y range [-1, 1], so the projection has to be adjusted
 */
class UnitGraph : public ContinuousGraph
{
public:
	
	/**
	 *	constructor
	 *	@param	numValues the number of unit values
	 *	@param	spacing the frequency of the units, eg. unit every minValue + n * spacing
	 *	@param	repeat if the units have to be repeated
	 *	@param	color the color of the graph
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 */
	UnitGraph(size_t numValues,
			  fPoint spacing,
			  bool repeat,
			  QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
			  float lineWidth = 1,
			  QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
			  const std::string& unit = std::string()
			  );
	
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
	 *	gets the number of grid values
	 *	@return	returns the number of grid values in the unit object
	 */
	size_t getNumberOfValues() const;
		
	/**
	 *	generates the index buffer for the given primitivetype.
	 *	this method also regenerates the vertexbuffer with the added data
	 */
	virtual void generateIndicesAndRegenerateVertexBuffer();

private:
	
	size_t	numValues_;		/**< the number of grid values. */
	fPoint	spacing_;		/**< the spacing. */
	bool	repeat_;		/**< if it will be repeated. */
	
	QVector4D color_;		/**< the color of the graph. */
	
	std::vector<int>	spacings_;	/**< the spacings of the levels. */
	
};

#endif //UNITGRAPH_HPP
