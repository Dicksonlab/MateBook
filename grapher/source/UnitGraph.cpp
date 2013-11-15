/*
 *  unitgraph.cpp
 *  grapher
 *
 *  Created by Herbert Grasberger on 12.08.11.
 *  Copyright 2011 by grasberger.org. All rights reserved.
 *
 */

#include "UnitGraph.hpp"
#include <cmath>

#include <iostream>

UnitGraph::UnitGraph(size_t numValues,
					 fPoint spacing,
					 bool repeat,
					 QVector4D color/* = QVector4D(0.0, 0.0, 0.0, 1.0)*/,
					 float lineWidth/* = 1*/,
					 QVector3D offset/* = QVector3D(0.0, 0.0, 0.0)*/, 
					 const std::string& unit/* = std::string()*/
					 ) :
ContinuousGraph((const fPoint*)NULL, (size_t)0, color, lineWidth, offset, unit),
numValues_(numValues), spacing_(spacing), repeat_(repeat), color_(color), spacings_()
{
	generateIndicesAndRegenerateVertexBuffer();
}

void UnitGraph::draw(QMatrix4x4 & view,
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
	int64_t valuesDisplayed = rightValue - leftValue;
	float multiplier = (float)valuesDisplayed / numValues_;
	
	/**
	 *	figuring out how many of our units need to be displayed (how fine grained we want to see the units)
	 */
	float level = ((float)log((float)(valuesDisplayed - 1) / 2.0f)) / log(spacing_ * 10.0f);
	
	
	/**
	 *	setting the color
	 */
	glColor4f(color_.x(), color_.y(), color_.z(), color_.w());
	
	/**
	 *	drawing the x axis
	 */
	glBegin(GL_LINES);
	
	glVertex3f(leftValue, 0.0f, 0.0f);
	glVertex3f(rightValue, 0.0f, 0.0f);
	
	glEnd();
	
	
	glPushMatrix();
	
	glTranslatef(offset_.x(), offset_.y(), offset_.z());
	
	float amax = std::max(fabs(maxValue), fabs(minValue));
	
	glLineWidth(lineWidth_);
	
	/**
	 *	calculating the start and end level we want to see
	 */
	int startLevel = std::max(0, std::min((int)vertexBuffers_.size() - 1, (int)level - 1));
	int endLevel = std::min((int)vertexBuffers_.size(), startLevel + 2);
	
//	std::cout << startLevel << " " << endLevel << std::endl;
//	endLevel = startLevel + 1;
	
	/**
	 *	drawing the levels
	 */
	for(int i = startLevel; i < endLevel ; ++i)
	{
		VertexBuffer * current = vertexBuffers_[vertexBuffers_.size() - 1 - i];
		
		int firstIndex = std::floor(leftValue / (numValues_ * spacing_));
		int lastIndex = std::floor(rightValue / (numValues_ * spacing_));
		
		current->bind();
		
		if(i != startLevel)
		{
			/**
			 *	the level with the maximum visiblity,
			 */
			glColor4f(color_.x(), color_.y(), color_.z(), color_.w());
			
			for(int j = firstIndex; j <= lastIndex; ++j)
			{
				
				glPushMatrix();
				
				glScalef(1.0f, amax / 10.0f, 1.0f);
				
				glTranslatef((float)(j * (numValues_ * spacing_)), 0.0f, 0.0f);
				
				
				glDrawElements(GL_LINES, current->getNumberOfIndices(), GL_UNSIGNED_INT, (void*)0);
				
				
				glPopMatrix();
			}
			
			
			glColor4f(color_.x(), color_.y(), color_.z(), color_.w() * 0.3f);
		}
		else
		{
			/**
			 *	the disappearing color of the units not displayed at full visibility
			 */
			float modifier = ((level - startLevel) > 2) ? 0.0 : (1.0f - (level - (int)level));
			
			glColor4f(color_.x(), color_.y(), color_.z(), color_.w() * modifier * 0.3f);
		}
		
		for(int j = firstIndex; j <= lastIndex; ++j)
		{
		
			/**
			 *	shifiting our units along the x axes until the whole screen is covered with them
			 */
			glPushMatrix();
			
			glScalef(1.0f, amax, 1.0f);
			
			glTranslatef((float)(j * (numValues_ * spacing_)), 0.0f, 0.0f);
			
			
			glDrawElements(GL_LINES, current->getNumberOfIndices(), GL_UNSIGNED_INT, (void*)0);
			
			
			glPopMatrix();
		}
		
		
		current->release();
	}
	glPopMatrix();
}

void UnitGraph::generateIndicesAndRegenerateVertexBuffer()
{
	/**
	 *	generate a certain number of unit levels, all a factor of 10.
	 */
	float levels = ((float)log((float)numValues_)) / log(spacing_ * 10.0f);
	
	/**
	 *	every level is a vertexbuffer
	 */
	for(int i = (int) levels - 1; i >= 0; --i)
	{
		int levelSpacing = pow(spacing_ * 10.0f, i);		
		spacings_.push_back(levelSpacing);
		
		std::vector<int> curIndices;
		std::vector<QVector3D> curPos;
		curIndices.reserve(numValues_ / levelSpacing);
		curPos.reserve(numValues_ / levelSpacing);
		
		/**
		 *	setting the vertices for the unit lines
		 */
		for(size_t j = 0; j < numValues_; j += levelSpacing)
		{
			/**
			 *	line
			 */
			curPos.push_back(QVector3D((float)j, -1.0, 0.0));
			curPos.push_back(QVector3D((float)j, 1.0, 0.0));

			/**
			 *	corresponding index
			 */
			curIndices.push_back((j / levelSpacing) * 2);
			curIndices.push_back((j / levelSpacing) * 2 + 1);
		}
		
		/**
		 *	create vertexbuffer
		 */
		VertexBuffer * current = new VertexBuffer(curIndices, curPos);
		current->setXMin(0);
		current->setXMax(numValues_ * spacing_);
		
		vertexBuffers_.push_back(current);
	}
}
