/*
 *  markerdata.cpp
 *  grapher
 *
 *  Created by Herbert Grasberger on 21.11.10.
 *  Copyright 2010 by grasberger.org. All rights reserved.
 *
 */
#if defined(WIN32) || defined(LINUX)
  #include <GL/glu.h>
  #include <GL/gl.h>
#else
  #include <QtOpenGL>
#endif

#include "MarkerGraph.hpp"
#include "QGLGrapher.hpp"

#include <QGLWidget>
#include <QMatrix4x4>

#include <math.h>
#include <iostream>
#include <limits>


MarkerGraph::MarkerGraph(QImage image,
						 QGLGrapher * context,
						 size_t imageWidth,
						 size_t imageHeight,
						 const fPoint* x,
						 const fPoint* y,
						 size_t count,
						 QVector3D offset/* = QVector3D(0.0, 0.0, 0.0)*/,
						 float scale/* = 1.0f*/,
						 float rotate/* = 0.0f*/) : 
AbstractGraph(offset),
baseImage_(image),
x_(x),
y_(y),
count_(count),
scale_(scale),
rotate_(rotate),
maxValue_(0),
minValue_(0)
{
	/**
	 *	copying x and y value into our own storage
	 */
	std::copy(x, x + count, back_inserter(copiedX_));
	x_ = &copiedX_[0];
	std::copy(y, y + count, back_inserter(copiedY_));
	y_ = &copiedY_[0];
	
	/**
	 *	generating marker texture
	 */
	context->makeCurrent();
	markerInfo_.handle_ = context->bindTexture(baseImage_);
	
	markerInfo_.width_ = imageWidth;
	markerInfo_.height_ = imageHeight;
	
	/**
	 *	calculating min/max values
	 */
	maxValue_ = std::numeric_limits<fPoint>::min();
	minValue_ = std::numeric_limits<fPoint>::max();
	
	for(size_t i = 0; i < count_; ++i)
	{
		if(y_[i] > maxValue_)
		{
			maxValue_ = y_[i];
		}
		if(y_[i] < minValue_)
		{
			minValue_ = y_[i];
		}
	}

}

MarkerGraph::~MarkerGraph()
{

	x_ = NULL;
	y_ = NULL;
	glDeleteTextures(1,  & (markerInfo_.handle_));
}

void MarkerGraph::draw(QMatrix4x4 & view,
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
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	/**
	 *	translating the markergraph to its actual position
	 */
	QMatrix4x4 modelView;
	modelView = view;
	modelView.translate(offset_);
	glLoadMatrixd(modelView.data());
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, markerInfo_.handle_);
//	glColor3f(1.0f, 0.0f, 0.0f);
	
	float pixelWidth;
	float pixelHeight;
	
	/**
	 *	calculating the size of the marker for the current zoom factor
	 */
	if(rotated)
	{
		pixelWidth = (rightValue - leftValue) / height;
		pixelHeight = (maxValue - minValue) / width;
	}
	else
	{
		pixelWidth = (rightValue - leftValue) / width;
		pixelHeight = (maxValue - minValue) / height;
	}
	
	
	float markerWidth = markerInfo_.width_ * pixelWidth;
	float markerHeight = markerInfo_.height_ * pixelHeight;
	
	
	float halfImageWidth = (markerWidth * scale_) / 2.0f;
	float halfImageHeight = (markerHeight * scale_) / 2.0f;
	
	std::cout << zoomFactor << std::endl;
	
	if(rotated)
	{
		halfImageWidth *= zoomFactor;
	}
	else
	{
		halfImageHeight *= zoomFactor;
	}

	/**
	 *	drawing the markers
	 */
	for(size_t i = 0; i < count_; ++i)
	{
		float curx = x_[i];
		
		/**
		 *	only if markers are in visible region
		 */
		if(curx >= leftValue && curx <= rightValue)
		{
			glPushMatrix();
			
			float y = y_[i];
			/**
			 *	doing the actual scale and translate of the marker,
			 *	drawn as quad, to the actual position
			 */
			glTranslatef(curx, y, 0.0f);			
			glScalef(halfImageWidth, halfImageHeight, 1.0f);
			if(rotated)
			{
				glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
			}
			glRotatef(rotate_, 0.0f, 0.0f, 1.0f);
			
			
			glBegin(GL_QUADS);
			
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.0f, -1.0f, 0.0f);
			
			
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.0f, -1.0f, 0.0f);
			
			
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.0f, 1.0f, 0.0f);
			
			
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-1.0f, 1.0f, 0.0f);
			
			glEnd();
			
			glPopMatrix();
		}
	}
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
}

const QVector3D * MarkerGraph::getDataPtr() const
{
	return NULL;
}

fPoint MarkerGraph::getMaxValue() const
{
	return maxValue_;
}

fPoint MarkerGraph::getMinValue() const
{
	return minValue_;
}

std::string MarkerGraph::getUnits() const
{
	return "";
}

size_t MarkerGraph::getSize() const
{
	return 0;
}
