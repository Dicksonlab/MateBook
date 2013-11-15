#ifndef GraphicsPrimitives_hpp
#define GraphicsPrimitives_hpp

#include <QGLWidget>

class Video;

class GraphicsPrimitives
{
public:
	static void drawLine(float startX, float startY, float endX, float endY);
	static void drawQuad(float left, float top, float width, float height);
	static void drawQuadContour(float left, float top, float width, float height);
	static void drawQuadTextured(float left, float top, float width, float height);
	static void drawArrow(float length, float breadth);
	static void drawCross();

private:
};

#endif
