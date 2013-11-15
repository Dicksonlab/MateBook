#ifndef FancyMenu_hpp
#define FancyMenu_hpp

#include <QMenu>
#include <QAction>
#include <QModelIndex>
#include <QModelIndexList>

class FancyMenu : public QMenu {
	Q_OBJECT

public:
	FancyMenu(QWidget* parent = 0);
	virtual ~FancyMenu();

	virtual QAction* exec(const QPoint& p, const QModelIndex& currentIndex, const QModelIndexList& selection, QAction* action = 0);
};

#endif
