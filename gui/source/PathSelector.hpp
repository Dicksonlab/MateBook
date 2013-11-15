#ifndef PathSelector_hpp
#define PathSelector_hpp

#include <QLineEdit>

class QToolButton;

class PathSelector : public QLineEdit
{
	Q_OBJECT

public:
	PathSelector(QWidget* parent = 0);

protected:
	void resizeEvent(QResizeEvent*);

private slots:
	void selectDirectory();

private:
	QToolButton* selectorButton;
};

#endif
