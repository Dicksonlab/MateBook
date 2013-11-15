#ifndef ItemMenu_hpp
#define ItemMenu_hpp

#include "FancyMenu.hpp"

class FileItem;

class ItemMenu : public FancyMenu {
	Q_OBJECT

public:
	ItemMenu(QWidget* parent = 0);
	~ItemMenu();

	QAction* exec(const QPoint& p, const QModelIndex& currentIndex, const QModelIndexList& selection, QAction* action = 0);

private slots:
	void openResultDirectory();
	void showSettingsUsed();

private:
	QAction* openResultDirectoryAction;
	QAction* showSettingsUsedAction;
	FileItem* item;

	void createActions();
};

#endif
