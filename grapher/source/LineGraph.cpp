#if defined(WIN32) || defined(LINUX)
	#include <GL/glu.h>
	#include <GL/gl.h>
#else
	#include <QtOpenGL>
#endif

#include "LineGraph.hpp"

#include <QGLWidget>
#include <iostream>
#include <QMatrix4x4>

void LineGraph::draw(QMatrix4x4 & view,
					 QMatrix4x4 & projection,
					 size_t width, 
					 size_t height,
					 fPoint maxValue,
					 fPoint minValue,
					 fPoint leftValue,
					 fPoint rightValue,
					 bool rotated,
					 float zoomFactor)
{
	glPushMatrix();
	
	QMatrix4x4 modelView;
	
	glLineWidth(lineWidth_);
	
	/**
	 *	looping over all the tiles of our graph
	 */
	for(size_t i = 0; i < vertexBuffers_.size() ; ++i)
	{
		VertexBuffer * current = vertexBuffers_[i];
		
		/**
		 *	checking if the current graph tile is actually visible
		 */
		float xmin = offset_.x() + current->getXMin();
		float xmax = offset_.x() + current->getXMax();
		
		if(xmax < leftValue || xmin > rightValue)
		{
			//std::cout << xmax << " " << xmin << std::endl;
			continue;
		}
		else
		{
			/**
			 *	translating the tile to its proper position
			 */
			modelView = view;
			modelView.translate((offset_ + current->getBufferOffset()));
			glLoadMatrixd(modelView.data());
			
			/**
			 *	drawing the whole vertexbuffer
			 */
			current->bind();
			glDrawElements(GL_LINE_STRIP, current->getNumberOfIndices(), GL_UNSIGNED_INT, (void*)0);
			current->release();
		}
		
		GLenum error = glGetError();
		
		if(error != GL_NO_ERROR)
		{
			std::cout << gluErrorString(error) << std::endl;
		}

		
	}
	glPopMatrix();
}

void LineGraph::generateIndicesAndRegenerateVertexBuffer()
{
	/**
	 *	builing the vertex buffer(s) from a given data set
	 *	depending on ContinousGraph::splitThreshold we might end up with
	 *	several vertex buffers, that are defined between x = [0, splitThreshold], and then
	 *	moved into place
	 */
	std::vector<int> curIndices;
	std::vector<QVector3D> curPos;
	std::vector<QVector4D> curColors;
	
	size_t reserveValues = std::min(positions_.size(), ContinuousGraph::splitThreshold);
	
	curIndices.reserve(reserveValues);
	curPos.reserve(reserveValues);
	curColors.reserve(reserveValues);
	
	size_t counter = 0;
	size_t min = 0;
	size_t offsetIndex = 0;
	fPoint offsetValue = 0;
	
	QVector3D tmpPos;
	
	/**
	 *	generating the positions. this time the actual (x,y) value and the corresponding color
	 */
	for(size_t i = 0; i < positions_.size(); ++i)
	{
		curIndices.push_back((int)i - min);
		tmpPos = positions_[i];
		tmpPos.setX(tmpPos.x() - offsetValue);
		curPos.push_back(tmpPos);
		curColors.push_back(colors_[i]);
		
		counter++;
		
		/**
		 *	encountered a split
		 */
		if((counter == ContinuousGraph::splitThreshold) && (i != (positions_.size() - 1)))
		{
			/**
			 *	create the vertexbuffer from the current data
			 */
			VertexBuffer * current = new VertexBuffer(curIndices, curPos, curColors);
			current->setXMin(min);
			current->setXMax(i);
			min = i;
			QVector3D bufferOffset(offsetValue, 0.0f, 0.0f);
			current->setBufferOffset(bufferOffset);
			
			/**
			 *	incremented the values for next vertexbuffer
			 */
			++offsetIndex;
			offsetValue = offsetIndex * ContinuousGraph::splitThreshold;
			
			vertexBuffers_.push_back(current);
			
			/**
			 *	initialise next vertex buffer values
			 */
			std::vector<int> newIndices;
			std::vector<QVector3D> newPos;
			std::vector<QVector4D> newColors;
			
			newIndices.reserve(reserveValues);
			newPos.reserve(reserveValues);
			newColors.reserve(reserveValues);
			
			
			curIndices.swap(newIndices);
			curPos.swap(newPos);
			curColors.swap(newColors);
			
			curIndices.push_back(0);
			tmpPos = positions_[i];
			tmpPos.setX(tmpPos.x() - offsetValue);
			curPos.push_back(tmpPos);
			curColors.push_back(colors_[i]);
			
			counter = 0;
		}
		
	}	
	
	/**
	 *	set the last vertexbuffer
	 */
	VertexBuffer * current = new VertexBuffer(curIndices, curPos, curColors);
	current->setXMin(min);
	current->setXMax(positions_.size());
	QVector3D bufferOffset(offsetValue, 0.0f, 0.0f);
	current->setBufferOffset(bufferOffset);
	
	vertexBuffers_.push_back(current);
}
