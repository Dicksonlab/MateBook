#ifndef VerticalWidgetList_hpp
#define VerticalWidgetList_hpp

/*
The callback function passed to the constructor is called whenever there's a new widget to be created. It may return NULL; if it does, no widget is added.
The reason we opted for such an obscure widget creation mechanism is that Qt doesn't allow Q_OBJECTs to be templates,
so we cannot specify the type of widget to be created at compile-time without resorting to typedef-followed-by-#include or #define-followed-by-#include tricks that are error-prone.
There's also no clone() or copy constructor defined for QWidget.
*/

#include <QWidget>
#include <QList>
#include <boost/function.hpp>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

class VerticalWidgetList : public QWidget
{
	Q_OBJECT

public:
	VerticalWidgetList(boost::function<QWidget* ()> widgetCreator, QWidget* parent = 0);
	~VerticalWidgetList();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	QWidget* getWidget(int widgetIndex);
	int count() const;

	void setContentAlignment(Qt::Alignment alignment);

public slots:
	void setWidgetCount(int widgetCount);

private slots:
	void removeRow();
	void addRow();
	void moveRowUp();
	void moveRowDown();

private:
	QWidget* makeDummy() const;	// create a top-level widget to be added as a row in the verticalLayout; the caller takes ownership

	boost::function<QWidget* ()> widgetCreator;	// gets called whenever there's a new widget to be added
	QVBoxLayout* verticalLayout;
};

#endif
