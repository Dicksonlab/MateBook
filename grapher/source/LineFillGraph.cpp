/*
 *  linefillgraphdata.cpp
 *  grapher
 *
 *  Created by Herbert Grasberger on 04.07.11.
 *  Copyright 2011 by grasberger.org. All rights reserved.
 *
 */

#include "LineFillGraph.hpp"

#include <QGLWidget>
#include <cmath>
#include <iostream>
#include <QMatrix4x4>

void LineFillGraph::draw(QMatrix4x4 & view,
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
			glDrawElements(GL_TRIANGLES, current->getNumberOfIndices(), GL_UNSIGNED_INT, (void*)0);
			current->release();
		}
		
		
	}
	glPopMatrix();
}

#define SIGN(_a)		((_a) > 0 ? 1 : (_a) < 0 ? -1 : 0)

void LineFillGraph::generateIndicesAndRegenerateVertexBuffer()
{
	/**
	 *	builing the vertex buffer(s) from a given data set
	 *	depending on ContinousGraph::splitThreshold we might end up with
	 *	several vertex buffers, that are defined between x = [0, splitThreshold], and then
	 *	moved into place
	 */
	std::vector<std::vector<int> > allIndices;
	std::vector<std::vector<QVector3D> > allPositions;
	std::vector<std::vector<QVector4D> > allColors;
	std::vector<size_t> allMins;
	std::vector<size_t> allMaxs;
	std::vector<size_t> prevPos;
	std::vector<QVector3D> allBufferOffsets;
	
	std::vector<int> curIndices;
	std::vector<QVector3D> curPos;
	std::vector<QVector4D> curColors;
	
	
	size_t reserveValues = std::min(positions_.size(), ContinuousGraph::splitThreshold);
	
	curIndices.reserve(reserveValues);
	curPos.reserve(reserveValues);
	curColors.reserve(reserveValues);
	
	int64_t counter = 0;
	size_t min = 0;
	size_t offsetIndex = 0;
	fPoint offsetValue = 0;
	
	QVector3D tmpPos;
	
	/**
	 *	generating the positions. this time the actual (x,y) value and the corresponding color
	 */
	for(size_t i = 0; i < positions_.size() - 1; ++i)
	{
		tmpPos = positions_[i];
		tmpPos.setX(tmpPos.x() - offsetValue);
		curPos.push_back(tmpPos);
		curColors.push_back(QVector4D(colors_[i].x(), colors_[i].y(), colors_[i].z(), fillAlpha_));
		
		counter++;
		
		/**
		 *	encountered a split
		 */
		if((counter == ContinuousGraph::splitThreshold) && (i != (positions_.size() - 1)))
		{
			/**
			 *	add the starting values for the next split part, so we don't get holes in between vertexbuffers
			 */
			tmpPos = positions_[i + 1];
			tmpPos.setX(tmpPos.x() - offsetValue);
			curPos.push_back(tmpPos);
			curColors.push_back(QVector4D(colors_[i + 1].x(), colors_[i + 1].y(), colors_[i + 1].z(), fillAlpha_));
			
			/**
			 *	storing the values of the now previous vertexbuffer
			 */
			allPositions.push_back(curPos);
			allColors.push_back(curColors);
			allBufferOffsets.push_back(QVector3D(offsetValue, 0.0f, 0.0f));
			++offsetIndex;
			offsetValue = offsetIndex * ContinuousGraph::splitThreshold;
			
			prevPos.push_back(curPos.size());
			
			/**
			 *	next values initialised
			 */
			curPos = std::vector<QVector3D>();
			curColors = std::vector<QVector4D>();
			
			curPos.reserve(reserveValues);
			curColors.reserve(reserveValues);
			
			allMins.push_back(min);
			allMaxs.push_back(i);
			counter = 0;
			
			min = i;
			
			
		}
		
	}
	
	/**
	 *	storing the values of the last vertexbuffer
	 */
	allPositions.push_back(curPos);
	allColors.push_back(curColors);
	allMins.push_back(min);
	allMaxs.push_back(positions_.size());
	allBufferOffsets.push_back(QVector3D(offsetValue, 0.0f, 0.0f));
	offsetIndex = 0;
	offsetValue = 0;
	
	prevPos.push_back(curPos.size());
	
	
	
	size_t index = 0;
	std::vector<std::vector<QVector3D> >::iterator singlePos = allPositions.begin() + index;
	std::vector<std::vector<QVector4D> >::iterator singleColor = allColors.begin() + index;
	
	counter = 0;
	
	/**
	 *	loop 2: adding the correspoding (x,0) value at the end of every array, same for color
	 */
	for(size_t i = 0; i < std::max(positions_.size() - 1, (size_t)0); ++i)
	{
		(*singlePos).push_back(QVector3D((float)i - offsetValue, 0.0f, 0.0f));
		(*singleColor).push_back(QVector4D(colors_[i].x(), colors_[i].y(), colors_[i].z(), fillAlpha_));
		
		counter++;
		
		/**
		 *	encountered split
		 */
		if(counter == ContinuousGraph::splitThreshold)
		{
			/**
			 *	adding first value of next vertexbuffer at end, so we are complete
			 */
			(*singlePos).push_back(QVector3D((float)(i + 1) - offsetValue, 0.0f, 0.0f));
			(*singleColor).push_back(QVector4D(colors_[i + 1].x(), colors_[i + 1].y(), colors_[i + 1].z(), fillAlpha_));

			++offsetIndex;
			offsetValue = offsetIndex * ContinuousGraph::splitThreshold;
			
			/**
			 *	switching to the data for the next vertexbuffer
			 */
			index++;
			singlePos = allPositions.begin() + index;
			singleColor = allColors.begin() + index;
			
			counter = 0;
			
		}
	}
	
		
	allIndices.resize(allPositions.size());
	
	/**
	 *	loop 3: generating the indices
	 */
	for(size_t j = 0; j < allPositions.size(); ++j)
	{
		/**
		 *	looping over all values in positions
		 */
		size_t prevPosSize = prevPos[j];
		for(size_t i = 0; i < (prevPosSize - 1); ++i)
		{
			/**
			 *	if our graph intersects the x axis, we need to add a point at the intersection
			 */
			if(SIGN(allPositions[j][i].y() / maxValue_) != SIGN(allPositions[j][i + 1].y() / maxValue_))
			{
				/**
				 *	interpolate position and color at the x intersection and add them to the end of the arrays
				 */
				float difference = (fabs(allPositions[j][i].y())+ fabs(allPositions[j][i + 1].y())) / maxValue_;
				float interpolator = (fabs(allPositions[j][i].y() / maxValue_) / difference);
				float midPoint = (float)i + interpolator;
				
				QVector4D midColor = (1.0f - interpolator) * allColors[j][i] + (allColors[j][i + 1]) * interpolator;
				
				allPositions[j].push_back(QVector3D(midPoint, 0.0f, 0.0f));				
				allColors[j].push_back(midColor);
				
				/**
				 *	building the indexbuffer for the two triangles
				 *	left and right of x intersection
				 */
				allIndices[j].push_back(i);
				allIndices[j].push_back(allPositions[j].size() - 1);
				allIndices[j].push_back(i + prevPosSize);
				
				allIndices[j].push_back(allPositions[j].size() - 1);
				allIndices[j].push_back(i + prevPosSize + 1);
				allIndices[j].push_back(i + 1);
				
			}
			else
			{
				/**
				 *	building the indexbuffer for the two triangles
				 *	forming the filled region under the curve
				 */
				allIndices[j].push_back(i + prevPosSize);
				allIndices[j].push_back(i + prevPosSize + 1);
				allIndices[j].push_back(i);
				
				allIndices[j].push_back(i);
				allIndices[j].push_back(i + prevPosSize + 1);
				allIndices[j].push_back(i + 1);

			}
		}
		
		/**
		 *	done with the indices, build the actual vertexbuffer
		 */
		VertexBuffer * current = new VertexBuffer(allIndices[j], allPositions[j], allColors[j]);
		current->setXMin(allMins[j]);
		current->setXMax(allMaxs[j]);
		current->setBufferOffset(allBufferOffsets[j]);
		
		vertexBuffers_.push_back(current);
	}
	
}
