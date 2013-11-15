#ifndef AttributeSelector_hpp
#define AttributeSelector_hpp

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class QGroupBox;
QT_END_NAMESPACE

class ColorButton;

class AttributeSelector : public QWidget
{
	Q_OBJECT

public:
	AttributeSelector(QWidget* parent = 0);
	virtual ~AttributeSelector();

//	QSize minimumSizeHint() const;
//	QSize sizeHint() const;

	std::string getString() const;	// a string describing this widget's value
	void fromString(const std::string& description);	// set this widget's state from the string

	void selectAttribute(const QString& name, const QString& kind);
	void setColor(const QColor& color);

signals:

public slots:
	void reset();

private slots:

private:
	QComboBox* attributeComboBox;
	ColorButton* colorButton;
};

#endif
