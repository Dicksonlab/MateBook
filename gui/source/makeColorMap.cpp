#include "makeColorMap.hpp"
#include <QColor>

std::vector<QVector4D> makeColorMap(size_t size)
{
	std::vector<QVector4D> ret;

	if (size == 0) {
		return ret;
	}

	double hue = 200.0 / 360.0;
	for (size_t colorNumber = 0; colorNumber != size; ++colorNumber) {
		QColor color = QColor::fromHsvF(hue, 1, 1);
		ret.push_back(QVector4D(color.redF(), color.greenF(), color.blueF(), 1));
		hue += 115.0 / 360.0;
		if (hue >= 1) {
			hue -= 1;
		}
	}

	return ret;
}
