#if defined(WIN32)
	#include <GL/glew.h>
#else
	#include <QtOpenGL>
#endif

#include "VertexBuffer.hpp"


VertexBuffer::VertexBuffer(const std::vector<int> & indices, const std::vector<QVector3D> & positions) : indices_(indices), indexBuffer_(NULL), positions_(positions), positionBuffer_(NULL), colors_(NULL), colorBuffer_(NULL), texCoords_(), texCoordBuffer_(NULL)
{
}

VertexBuffer::VertexBuffer(const std::vector<int> & indices, const std::vector<QVector3D> & positions, const std::vector<QVector4D> &colors) : indices_(indices), indexBuffer_(NULL), positions_(positions), positionBuffer_(NULL), colors_(colors), colorBuffer_(NULL), texCoords_(), texCoordBuffer_(NULL)
{
}

VertexBuffer::VertexBuffer(const std::vector<int> & indices, const std::vector<QVector3D> & positions, const std::vector<QVector2D> &texCoords) : indices_(indices), indexBuffer_(NULL), positions_(positions), positionBuffer_(NULL), colors_(), colorBuffer_(NULL), texCoords_(texCoords), texCoordBuffer_(NULL)
{
}

VertexBuffer::~VertexBuffer()
{
	positions_.clear();
	colors_.clear();
	indices_.clear();
	texCoords_.clear();
	
	clear();
}

void VertexBuffer::bind()
{
	if(positionBuffer_ == NULL)
	{
		upload();
	}


	if(colorBuffer_)
	{
		glEnableClientState(GL_COLOR_ARRAY);
		colorBuffer_->bind();
		glColorPointer(4, GL_FLOAT, 0, 0);
	}

	if(texCoordBuffer_)
	{
		glActiveTexture(GL_TEXTURE0);
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		texCoordBuffer_->bind();
		glColorPointer(2, GL_FLOAT, 0, 0);
	}
	
	glEnableClientState(GL_VERTEX_ARRAY);
	positionBuffer_->bind();
	glVertexPointer(3, GL_FLOAT, 0, 0);
	
	indexBuffer_->bind();
}

void VertexBuffer::release()
{
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	
	if(texCoordBuffer_) texCoordBuffer_->release();
	if(colorBuffer_) colorBuffer_->release();
	if(positionBuffer_) positionBuffer_->release();
	if(indexBuffer_)indexBuffer_->release();
}

float VertexBuffer::getXMin() const
{
	return xMin_;
}

void VertexBuffer::setXMin(float m_Xmin)
{
	xMin_ = m_Xmin;
}

float VertexBuffer::getXMax() const
{
	return xMax_;
}

void VertexBuffer::setXMax(float m_Xmax)
{
	xMax_ = m_Xmax;
}

size_t VertexBuffer::getNumberOfIndices() const
{
	return indices_.size();
}

void VertexBuffer::setBufferOffset(QVector3D & bufferOffset)
{
	bufferOffset_ = bufferOffset;
}

const QVector3D & VertexBuffer::getBufferOffset() const
{
	return bufferOffset_;
}

void VertexBuffer::upload()
{
	positionBuffer_ = new QGLBuffer(QGLBuffer::VertexBuffer);
	if(positionBuffer_->create() && positionBuffer_->bind())
	{
		positionBuffer_->setUsagePattern(QGLBuffer::StaticDraw);
		positionBuffer_->allocate(&(positions_[0]), sizeof(QVector3D) * positions_.size());
		positionBuffer_->release();
	}	

	if(colors_.size() > 0)
	{
		colorBuffer_ = new QGLBuffer(QGLBuffer::VertexBuffer);
		if(colorBuffer_->create() && colorBuffer_->bind())
		{
			colorBuffer_->setUsagePattern(QGLBuffer::StaticDraw);
			colorBuffer_->allocate(&(colors_[0]), sizeof(QVector4D) * colors_.size());
			colorBuffer_->release();
		}
	}
	
	if(texCoords_.size() > 0)
	{
		texCoordBuffer_ = new QGLBuffer(QGLBuffer::VertexBuffer);
		if(texCoordBuffer_->create() && texCoordBuffer_->bind())
		{
			texCoordBuffer_->setUsagePattern(QGLBuffer::StaticDraw);
			texCoordBuffer_->allocate(&(texCoords_[0]), sizeof(QVector2D) * texCoords_.size());
			texCoordBuffer_->release();
		}
	}


	indexBuffer_ = new QGLBuffer(QGLBuffer::IndexBuffer);
	if(indexBuffer_->create() && indexBuffer_->bind())
	{
		indexBuffer_->setUsagePattern(QGLBuffer::StaticDraw);
		indexBuffer_->allocate(&(indices_[0]), sizeof(int) * indices_.size());
		indexBuffer_->release();
	}
}

#define FREE(_x) if(_x) \
{ \
_x->destroy(); \
delete _x; \
_x = NULL; \
}

void VertexBuffer::clear()
{
	FREE(positionBuffer_);
	FREE(colorBuffer_);
	FREE(texCoordBuffer_);
	FREE(indexBuffer_);

}
