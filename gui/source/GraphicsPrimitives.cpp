#include "GraphicsPrimitives.hpp"

#include <cmath>

void GraphicsPrimitives::drawLine(float startX, float startY, float endX, float endY)
{
	glBegin(GL_LINES);
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glVertex3f(startX, startY, 0);
		glVertex3f(endX, endY, 0);
	glEnd();
}

void GraphicsPrimitives::drawQuad(float left, float top, float width, float height)
{
	glBegin(GL_QUADS);
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glVertex3f(left, top, 0);
		glVertex3f(left + width, top, 0);
		glVertex3f(left + width, top + height, 0);
		glVertex3f(left, top + height, 0);
	glEnd();
}

void GraphicsPrimitives::drawQuadContour(float left, float top, float width, float height)
{
	glBegin(GL_LINE_LOOP);
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glVertex3f(left, top, 0);
		glVertex3f(left + width, top, 0);
		glVertex3f(left + width, top + height, 0);
		glVertex3f(left, top + height, 0);
	glEnd();
}

void GraphicsPrimitives::drawQuadTextured(float left, float top, float width, float height)
{
	glBegin(GL_QUADS);
		glColor4f(1.0, 1.0, 1.0, 0.5);
		glTexCoord2f(0, 0);
		glVertex3f(left, top, 0);
		glTexCoord2f(1, 0);
		glVertex3f(left + width, top, 0);
		glTexCoord2f(1, 1);
		glVertex3f(left + width, top + height, 0);
		glTexCoord2f(0, 1);
		glVertex3f(left, top + height, 0);
	glEnd();
}

void GraphicsPrimitives::drawArrow(float length, float breadth)
{
	glBegin(GL_TRIANGLES);
		glVertex3f(0, breadth / 2, 0);
		glVertex3f(0, -breadth / 2, 0);
		glVertex3f(length / 2, 0, 0);
		glVertex3f(-length / 2, breadth / 4, 0);
		glVertex3f(0, -breadth / 4, 0);
		glVertex3f(0, breadth / 4, 0);
		glVertex3f(-length / 2, breadth / 4, 0);
		glVertex3f(-length / 2, -breadth / 4, 0);
		glVertex3f(0, -breadth / 4, 0);
	glEnd();
}

void GraphicsPrimitives::drawCross()
{
	glBegin(GL_LINES);
		glVertex3f(-1, 0, 0);
		glVertex3f(1, 0, 0);
		glVertex3f(0, -1, 0);
		glVertex3f(0, 1, 0);
	glEnd();
}
