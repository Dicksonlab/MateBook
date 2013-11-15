/*
 *  continuousgraph.h
 *  grapher
 *
 *  Created by Herbert Grasberger on 15.11.10.
 *  Copyright 2010 by grasberger.org. All rights reserved.
 *
 */

#ifndef CONTINUOUSGRAPH_H
#define CONTINUOUSGRAPH_H


#include "AbstractGraph.hpp"
#include <string>
#include <limits>
#include "VertexBuffer.hpp"
#include "precision.hpp"

/**
 *	@class	ContinuousGraph
 *	@brief	class representing a continuous set of graph data
 */
class ContinuousGraph : public AbstractGraph
{
public:
	
	static size_t splitThreshold;	/**< the threshold of values where it should be split up in several vertex buffers. */
	
	/**
	 *	constructor
	 *	only needed to construct a UnitGraph object
	 *	@param	data the data to be displayed
	 *	@param	dataSize the number of values
	 *	@param	color the color of the graph
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns the id of this object
	 */
	ContinuousGraph(const fPoint * data,
					size_t dataSize,
					QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
					float lineWidth = 1,
					QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
					const std::string& unit = std::string()
					);
	
	/**
	 *	constructor
	 *	@param	data the data to be displayed
	 *	@param	dataSize the number of values
	 *	@param	color the color of the graph
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 
	 *	@return	returns the id of this object
	 */
	template<class DataIterator>
	ContinuousGraph(	DataIterator dataBegin,
						DataIterator dataEnd,
						QVector4D color = QVector4D(0.0, 0.0, 0.0, 1.0),
						float lineWidth = 1,
						QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
						const std::string& unit = std::string()
					)  :
	AbstractGraph(offset),
	indices_(),
	positions_(),
	unit_(unit),
	colors_(),
	maxValue_(-std::numeric_limits<fPoint>::infinity()),
	minValue_(std::numeric_limits<fPoint>::infinity()),
	vertexBuffers_(),
	lineWidth_(lineWidth)
	{
		convert(dataBegin, dataEnd, &color, 1);
		calculateExtremeValues();
	}

	
	/**
	 *	constructor with given values
	 *	@param	data the data to be displayed
	 *	@param	dataSize the number of values
	 *	@param	colors the individual colors of values
	 *	@param	lineWidth the line width in OpenGL units
	 *	@param	offset the offset of the graph
	 *	@param	unit the unit 	
	 */
	template<class DataIterator, class ColorIterator>
	ContinuousGraph(	DataIterator dataBegin,
						DataIterator dataEnd,
						ColorIterator colorsBegin,
						float lineWidth = 1,
						QVector3D offset = QVector3D(0.0, 0.0, 0.0), 
						const std::string& unit = std::string()
					) :
	AbstractGraph(offset),
	indices_(),
	positions_(),
	unit_(unit),
	colors_(),
	maxValue_(-std::numeric_limits<fPoint>::infinity()),
	minValue_(std::numeric_limits<fPoint>::infinity()),
	vertexBuffers_(),
	lineWidth_(lineWidth)
	{
		convert(dataBegin, dataEnd, colorsBegin, dataEnd - dataBegin);
		calculateExtremeValues();
	}

	~ContinuousGraph();
	
	/*
	 * change graph color at a certain index on existing data
	 */
	template<class ColorIterator>
	void changeColor(ColorIterator colorsBegin, ColorIterator colorsEnd, size_t offset = 0)
	{
		for (size_t i = 0; offset + i < colors_.size() && colorsBegin != colorsEnd; ++i) {
			colors_[offset + i] = *(colorsBegin++);
		}
		
		for (size_t i = 0; i != vertexBuffers_.size(); ++i) {
			if (vertexBuffers_[i]) {
				delete vertexBuffers_[i];
				vertexBuffers_[i] = NULL;		
			}
		}
		vertexBuffers_.clear();
		
		//generateIndicesAndRegenerateVertexBuffer();
	}

	//virtual void setErrorBars(const std::vector<float>& errorData);

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

	/**
	 *	generates the index buffer for the given primitivetype.
	 *	this method also regenerates the vertexbuffer with the added data
	 */
	virtual void generateIndicesAndRegenerateVertexBuffer() = 0;
	
protected:
	
	/**
	 *	calculates the maximum and minimum value of the data set.
	 */
	void calculateExtremeValues();
	
	/**
	 *	@param	data the data
	 *	@param	numValues the number of values in data
	 *	@param	colors pointer to the color array
	 *	@param	numColors the number of colors. 
	 *			if numColors is smaller than numValues, the last color is repeated
	 *	@param	positions the new position data
	 *	@param	copiedColors the copied colors
	 */
	template<class DataIterator, class ColorIterator>
	void convert(DataIterator dataBegin, DataIterator dataEnd, ColorIterator colorsBegin, size_t numColors)
	{
		size_t numValues = dataEnd - dataBegin;
		positions_.resize(numValues);
		colors_.resize(numValues);
		
		for(size_t i = 0; i < numValues; ++i)
		{
			positions_[i] = QVector3D((float) i, dataBegin[i], 0.0f);
			if (i < numColors) {
				colors_[i] = colorsBegin[i];
			} else {
				colors_[i] = colorsBegin[numColors - 1];
			}
			
		}
	}

	
	
	std::vector<int>		indices_;			/**< the index data. */
	std::vector<QVector3D>	positions_;			/**< a copy of the data. */
	
	std::string				unit_;				/**< the units of the data set. */
	
	std::vector<QVector4D>	colors_;			/**< the colors per each data point, a copy. */
	
	fPoint					maxValue_;			/**< the maximum value in the data set. */
	fPoint					minValue_;			/**< the minimum value in the data set. */
	
	std::vector<VertexBuffer *> vertexBuffers_;	/**< the vertexbuffers. */
	float					lineWidth_;			/**< the line width. */
	
};

#endif // CONTINUOUSGRAPH_HPP
