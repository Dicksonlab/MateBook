/*
 *  vertexbuffer.h
 *  grapher
 *
 *  Created by Herbert Grasberger on 03.07.11.
 *  Copyright 2011 by grasberger.org. All rights reserved.
 *
 */

#ifndef VERTEXBUFFER_HPP
#define VERTEXBUFFER_HPP

#include <QGLWidget>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QGLBuffer>
#include <vector>

/**
 *	@class	VertexBuffer
 *	@brief	class representing an OpenGL vertexbuffer
 */
class VertexBuffer
{
public:

	/**
	 *	constructor
	 */
	VertexBuffer();
	
	/**
	 *	constructor with given value
	 *	@param	indices
	 *	@param	positions the data
	 */
	VertexBuffer(const std::vector<int> & indices, const std::vector<QVector3D> & positions);
	
	/**
	 *	constructor with given value
	 *	@param	indices
	 *	@param	positions the data
	 *	@param	colors pointer to the color array
	 */
	VertexBuffer(const std::vector<int> & indices, const std::vector<QVector3D> & positions, const std::vector<QVector4D> &colors);
	
	/**
	 *	constructor with given value
	 *	@param	indices
	 *	@param	positions the data
	 *	@param	texCoords pointer to the texture coordinate array
	 */
	VertexBuffer(const std::vector<int> & indices, const std::vector<QVector3D> & positions, const std::vector<QVector2D> &texCoords);
	
	/**
	 *	destructor
	 */
	~VertexBuffer();
	
	/**
	 *	binds the vertexbuffer
	 */
	void bind();
	
	/**
	 *	releases the vertexbuffer
	 */
	void release();
	
	/**
	 *	returns the minimum x value
	 *	@return	returns the value.
	 */
	float getXMin() const;
	
	/**
	 *	sets the minimum x value
	 *	@param	m_Xmin the value
	 */
	void setXMin(float m_Xmin);
		
	/**
	 *	returns the maximum x value
	 *	@return	returns the value.
	 */
	float getXMax() const;
	
	/**
	 *	sets the maximum x value
	 *	@param	m_Xmax the value
	 */
	void setXMax(float m_Xax);
	
	/**
	 *	returns the number of indices
	 *	@return	returns size of vector
	 */
	size_t getNumberOfIndices() const;
	
	/**
	 *	sets the offset of the current buffer
	 *	@param	bufferOffset the offset
	 */
	void setBufferOffset(QVector3D & bufferOffset);
	
	/**
	 *	gets the buffer offset 
	 *	@return	returns the buffer offset
	 */
	const QVector3D & getBufferOffset() const;
	
private:
	
	/**
	 *	uploads the data
	 */
	void upload();
	
	/**
	 *	clears the data in OpenGL
	 */
	void clear();
		
	std::vector<int>		indices_;			/**< the indices. */
	QGLBuffer				* indexBuffer_;		/**< the index buffer. */		
	
	std::vector<QVector3D>	positions_;			/**< the vertices stored on the CPU. Data only having y coordinates will be transformed to have (x,y,z). */
	QGLBuffer				* positionBuffer_;	/**< the position buffer. */
	
	std::vector<QVector4D>	colors_;			/**< the colors of each vertex. */
	QGLBuffer				* colorBuffer_;		/**< the color buffer. */
	
	std::vector<QVector2D>	texCoords_;			/**< the texture coordinates. */
	QGLBuffer				* texCoordBuffer_;	/**< the texture coordinate buffer. */
	
	float					xMin_;				/**< the minimum x value. */
	float					xMax_;				/**< the maximum x value. */
	
	QVector3D				bufferOffset_;		/**< the offset of the buffers coordinates in world coordinates. */
	
};


#endif //VERTEXBUFFER_HPP
