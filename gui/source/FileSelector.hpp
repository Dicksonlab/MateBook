#ifndef FileSelector_hpp
#define FileSelector_hpp

#include <QLineEdit>

class QToolButton;

class FileSelector : public QLineEdit
{
	Q_OBJECT

public:
	FileSelector(QWidget* parent = 0);

protected:
	void resizeEvent(QResizeEvent*);

private slots:
	void selectDirectory();

private:
	QToolButton* selectorButton;
};

#endif
