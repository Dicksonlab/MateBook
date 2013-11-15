/*
 *  whiskergraphdata.cpp
 *  grapher
 *
 *  Created by Herbert Grasberger on 04.07.11.
 *  Copyright 2011 by grasberger.org. All rights reserved.
 *
 */

#include "WhiskerGraph.hpp"

#include <QGLWidget>
#include <QMatrix4x4>

WhiskerGraph::~WhiskerGraph()
{
	errors_.clear();
}

void WhiskerGraph::draw(QMatrix4x4 & view,
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
			glDrawElements(GL_LINES, current->getNumberOfIndices(), GL_UNSIGNED_INT, (void*)0);
			current->release();
		}
		
		
	}
	glPopMatrix();
}

void WhiskerGraph::calculateExtremeValues()
{
	const QVector3D * dataPtr = & positions_[0];
    const QVector3D * dataEndPtr = dataPtr + positions_.size();
	
	for(size_t i = 0; i < positions_.size(); ++i)
	{
		QVector3D curMax = positions_[i] + errors_[i];
		QVector3D curMin = positions_[i] + errors_[i];
		
		if(curMax.y() > maxValue_)
		{
			maxValue_ = curMax.y();
		}
		if(curMin.y() < minValue_)
		{
			minValue_ = curMin.y();
		}
	}
}

void WhiskerGraph::generateIndicesAndRegenerateVertexBuffer()
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
	
	curIndices.reserve(reserveValues * 6);
	curPos.reserve(reserveValues * 6);
	curColors.reserve(reserveValues * 6);
	
	size_t counter = 0;
	size_t min = 0;
	size_t offsetIndex = 0;
	fPoint offsetValue = 0;
	
	QVector3D tmpPos;
	
	/**
	 *	generating the whiskers. they are built from 3 lines at the given (x,y), based on the error values
	 */
	for(size_t i = 0; i < positions_.size(); ++i)
	{
		tmpPos = positions_[i];
		tmpPos.setX(tmpPos.x() - offsetValue);
		
		/**
		 *	generating the positions
		 */
		curPos.push_back(QVector3D(tmpPos.x() - 0.5f, positions_[i].y() - errors_[i].y(), positions_[i].z()));
		curPos.push_back(QVector3D(tmpPos.x() + 0.5f, positions_[i].y() - errors_[i].y(), positions_[i].z()));		
		
		curPos.push_back(QVector3D(tmpPos.x(), positions_[i].y() - errors_[i].y(), positions_[i].z()));
		curPos.push_back(QVector3D(tmpPos.x(), positions_[i].y() + errors_[i].y(), positions_[i].z()));
		
		curPos.push_back(QVector3D(tmpPos.x() - 0.5f, positions_[i].y() + errors_[i].y(), positions_[i].z()));
		curPos.push_back(QVector3D(tmpPos.x() + 0.5f, positions_[i].y() + errors_[i].y(), positions_[i].z()));
		
		/**
		 *	duplicating colors for positions
		 */
		curColors.push_back(colors_[i]);
		curColors.push_back(colors_[i]);
		curColors.push_back(colors_[i]);
		curColors.push_back(colors_[i]);
		curColors.push_back(colors_[i]);
		curColors.push_back(colors_[i]);
		
		/**
		 *	indices too
		 */
		curIndices.push_back(counter * 6);
		curIndices.push_back(counter * 6 + 1);
		curIndices.push_back(counter * 6 + 2);
		curIndices.push_back(counter * 6 + 3);
		curIndices.push_back(counter * 6 + 4);
		curIndices.push_back(counter * 6 + 5);
		
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
			
			newIndices.reserve(reserveValues * 6);
			newPos.reserve(reserveValues * 6);
			newColors.reserve(reserveValues * 6);
			
			
			curIndices.swap(newIndices);
			curPos.swap(newPos);
			curColors.swap(newColors);
			
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
