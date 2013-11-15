/*
 *  continuousgraphdata.cpp
 *  grapher
 *
 *  Created by Herbert Grasberger on 15.11.10.
 *  Copyright 2010 by grasberger.org. All rights reserved.
 *
 */

#include "ContinuousGraph.hpp"

#include <QGLWidget>
#include <math.h>
#include <limits>

#define SIGN(_a)		((_a) > 0 ? 1 : (_a) < 0 ? -1 : 0)

size_t ContinuousGraph::splitThreshold = 1000000;

/**
 *	we need this constructor since the unit graph derives from ContinuousGraph and
 *	depends on this way to construct the graph
 */
ContinuousGraph::ContinuousGraph(const fPoint * data,
										 size_t dataSize,
										 QVector4D color,
										 float lineWidth,
										 QVector3D offset, 
										 const std::string& unit
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
	convert(data, data + dataSize, &color, 1);
	calculateExtremeValues();
}

ContinuousGraph::~ContinuousGraph()
{
	
	indices_.clear();
	positions_.clear();
	colors_.clear();
	
	for(size_t i = 0; i < vertexBuffers_.size(); ++i)
	{
		if(vertexBuffers_[i])
		{
			delete vertexBuffers_[i];
			vertexBuffers_[i] = NULL;		
		}
	}
	vertexBuffers_.clear();
}

const QVector3D * ContinuousGraph::getDataPtr() const
{
	return & positions_[0];
}

fPoint ContinuousGraph::getMaxValue() const
{
	return maxValue_;
}

fPoint ContinuousGraph::getMinValue() const
{
	return minValue_;
}

std::string ContinuousGraph::getUnits() const
{
	return unit_;
}

size_t ContinuousGraph::getSize() const
{
	return positions_.size();
}

void ContinuousGraph::calculateExtremeValues()
{
	maxValue_ = std::numeric_limits<fPoint>::min();
	minValue_ = std::numeric_limits<fPoint>::max();

	for(size_t i = 0; i < positions_.size(); ++i)
	{
		if(positions_[i].y() > maxValue_)
		{
			maxValue_ = positions_[i].y();
		}
		if(positions_[i].y() < minValue_)
		{
			minValue_ = positions_[i].y();
		}
	}
}
