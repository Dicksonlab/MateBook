#include "AbstractGraph.hpp"

AbstractGraph::AbstractGraph(QVector3D offset) : offset_(offset)
{
	
}

AbstractGraph::~AbstractGraph()
{
}


QVector3D AbstractGraph::getOffset() const
{
	return offset_;
}