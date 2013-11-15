#ifndef ColorButton_hpp
#define ColorButton_hpp

#include <QPushButton>
#include <QPixmap>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QColor;
QT_END_NAMESPACE

class ColorButton : public QPushButton
{
	Q_OBJECT

public:
	ColorButton(const QColor& color = Qt::black, QWidget* parent = 0);

	QColor getColor() const;

signals:
	void colorSelected(QColor color);

public slots:
	void setColor(QColor color);

private slots:
	void wasClicked();

private:
	QPixmap pixmap;
	QIcon icon;
	QColor currentColor;
};

#endif
