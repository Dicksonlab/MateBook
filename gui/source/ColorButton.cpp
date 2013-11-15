#include "ColorButton.hpp"

#include <QColorDialog>

ColorButton::ColorButton(const QColor& color, QWidget* parent) : QPushButton(parent),
	pixmap(32, 32)
{
	setColor(color);

	connect(this, SIGNAL(clicked()), this, SLOT(wasClicked()));
}

QColor ColorButton::getColor() const
{
	return currentColor;
}

void ColorButton::setColor(QColor color)
{
	pixmap.fill(color);
	icon = QIcon(pixmap);
	setIcon(icon);
	setText(color.name());
	currentColor = color;
}

void ColorButton::wasClicked()
{
	QColor selectedColor = QColorDialog::getColor(currentColor, this);
	if (selectedColor.isValid()) {
		setColor(selectedColor);
		emit colorSelected(selectedColor);
	}
}
